#include <file-parser.hh>
#include <engine.hh>
#include <configuration.hh>
#include <output-handler.hh>
#include <filter.hh>
#include <utils.hh>

#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/param.h>
#include <signal.h>

#include <list>
#include <unordered_map>
#include <vector>

#include <lldb/API/LLDB.h>
#include <lldb/API/SBUnixSignals.h>

using namespace kcov;
using namespace lldb;

extern char **environ;

class LLDBEngine : public IEngine, public IFileParser
{
public:
	LLDBEngine() :
		m_useLoadAddresses(false),
		m_listener(NULL),
		m_useLLDBBreakpoints(true),
		m_filter(NULL)
	{
		SBDebugger::Initialize();
		m_debugger = SBDebugger::Create();

		m_debugger.SetAsync(false);
		IParserManager::getInstance().registerParser(*this);
	}

	virtual ~LLDBEngine()
	{
		SBDebugger::Destroy(m_debugger);
	}

	// From IEngine
	virtual int registerBreakpoint(unsigned long addr)
	{
		if (m_useLLDBBreakpoints) {
			SBBreakpoint bp = m_target.BreakpointCreateByAddress(addr);

			if (!bp.IsValid())
				return -1;

			return bp.GetID();
		}

		// Use raw writes, faster but x86-only
		int data;

		// There already?
		if (m_instructionMap.find(addr) != m_instructionMap.end())
			return 0;

		data = peekByte(addr);
		m_instructionMap[addr] = data;
		pokeByte(addr, 0xcc);

		return m_instructionMap.size();
	}


	// From IFileParser
	virtual bool addFile(const std::string &filename, struct phdr_data_entry *phdr_data)
	{
		m_filename = filename;

		// This now assumes we have only one file, i.e., no shared libraries
		m_target = m_debugger.CreateTarget(m_filename.c_str());

		if (!m_target.IsValid())
			return false;


		for (FileListenerList_t::const_iterator it = m_fileListeners.begin();
				it != m_fileListeners.end();
				++it)
			(*it)->onFile(File(m_filename, IFileParser::FLG_NONE));

		return true;
	}

	virtual bool setMainFileRelocation(unsigned long relocation)
	{
		return true;
	}

	virtual void registerLineListener(ILineListener &listener)
	{
		m_lineListeners.push_back(&listener);
	}

	virtual void registerFileListener(IFileListener &listener)
	{
		m_fileListeners.push_back(&listener);
	}

	virtual bool parse()
	{
		// Parse the file/line -> address mappings
		for (uint32_t i = 0; i < m_target.GetNumModules(); i++)
		{
			SBModule cur = m_target.GetModuleAtIndex(i);

			if (!cur.IsValid())
				continue;

			handleModule(cur);
		}

		return true;
	}

	virtual uint64_t getChecksum()
	{
		return 0;
	}

	virtual enum IFileParser::PossibleHits maxPossibleHits()
	{
		return IFileParser::HITS_LIMITED;
	}

	virtual void setupParser(IFilter *filter)
	{
		m_filter = filter;
	}

	std::string getParserType()
	{
		return "lldb";
	}

	unsigned int matchParser(const std::string &filename, uint8_t *data, size_t dataSize)
	{
		return 1;
	}

	bool start(IEventListener &listener, const std::string &executable)
	{
		char buf[MAXPATHLEN + 1];
		SBError error;
		SBListener l;

		IConfiguration &conf = IConfiguration::getInstance();


		m_listener = &listener;

		if (getcwd(buf, sizeof(buf)) < 0) {
			error("No current WD?\n");

			return false;
		}

		// Configurable via the command line
		m_useLLDBBreakpoints = !conf.keyAsInt("lldb-use-raw-breakpoint-writes");

		unsigned int pid = conf.keyAsInt("attach-pid");

		if (pid != 0) {
			// Attach to running process, use load addresses in this case
			m_useLoadAddresses = true;

			m_process = m_target.AttachToProcessWithID(l, (lldb::pid_t)pid, error);
		} else {
			m_useLoadAddresses = false;

			// Launch process
			m_process = m_target.Launch(l,
					&IConfiguration::getInstance().getArgv()[1], // Minus the executable
					(const char **)environ,
					"/dev/stdin",
					"/dev/stdout",
					"/dev/stderr",
					buf,
					eLaunchFlagDisableASLR, // We can then use file addresses
					true, // Stop at entry point
					error);
		}

		if (!m_process.IsValid()) {
			error("Cannot launch process: %s\n", error.GetCString());

			return false;
		}

		if (error.Fail()) {
			error("Launch failure\n");

			return false;
		}

		// Special-case fork to workaround LLDB lack of follow-fork-mode
		m_forkBreakpoint = m_target.BreakpointCreateByName("fork");

		// Where the parent returns to after fork
		if (!m_forkBreakpoint.IsValid())
			warning("Can't set breakpoint at fork\n");

		// Send all signals to the process
		SBUnixSignals sigs = m_process.GetUnixSignals();
		for (unsigned int i = SIGHUP; i < SIGUSR2; i++) {
			sigs.SetShouldNotify(i, false);
			sigs.SetShouldStop(i, true);
			sigs.SetShouldSuppress(i, false);
		}

		return true;
	}

	bool continueExecution()
	{
		StateType state = m_process.GetState();
		Event ev;

		kcov_debug(BP_MSG, "STOPPED in state %d\n", state);

		switch (state)
		{
			case eStateStopped:
			{
				ev = handleStop();
			} break;

			case eStateExited:
			{
				ev = Event(ev_exit, m_process.GetExitStatus());
			} break;

			default:
				break;
		}

		m_listener->onEvent(ev);

		// Don't continue on exit (saves a lot of execution time)
		if (ev.type == ev_exit)
			return false;

		SBError err = m_process.Continue();

		return err.Success();
	}

	Event handleStop()
	{
		SBThread curThread = m_process.GetSelectedThread();
		SBFrame frame = curThread.GetSelectedFrame();

		enum StopReason stopReason = curThread.GetStopReason();
		unsigned long address = getAddress(frame.GetPCAddress());

		switch (stopReason)
		{
			case eStopReasonSignal:
			{
				int signo = (int)curThread.GetStopReasonDataAtIndex(0);
				enum event_type type = ev_signal;

				if (signo == SIGSEGV || signo == SIGABRT)
					type = ev_signal_exit;

				return Event(type, signo, address);

			} break;
			case eStopReasonBreakpoint:
			{
				uint64_t id = curThread.GetStopReasonDataAtIndex(0);

				/*
				 * Special case for forks (OSX-specific):
				 *
				 * 1. Disable all breakpoints so that the child doesn't stop on INT3
				 * 2. Set a temporary breakpoint in the parent fork return location
				 * 3. Continue to this point. We have now started the child and are back in the parent.
				 * 4. Enable all breakpoints again
				 *
				 */
				if (m_forkBreakpoint.IsValid() && id == m_forkBreakpoint.GetID()) {
					m_target.DisableAllBreakpoints();

					SBBreakpoint inParent = m_target.BreakpointCreateByName("libSystem_atfork_parent");
					if (inParent.IsValid()) {
						m_process.Continue();
						m_target.BreakpointDelete(inParent.GetID());
					}

					m_target.EnableAllBreakpoints();
				} else {
					m_target.BreakpointDelete(id);
				}
			} break;
			case eStopReasonException:
			{
				uint64_t id = curThread.GetStopReasonDataAtIndex(0);

				// Remove breakpoint by restoring the instruction again
				address--;
				InstructionMap_t::iterator it = m_instructionMap.find(address);

				if (id == 6 && it != m_instructionMap.end()) {
					pokeByte(address, m_instructionMap[address]);
					frame.SetPC(address);
				} else {
					// Can't find this breakpoint or unknown exception type?
					return Event(ev_error, -1, address);
				}
			} break;

			default:
				kcov_debug(BP_MSG, "Unknown stop reason %d\n", stopReason);
				break;
		}

		return Event(ev_breakpoint, -1, address);
	}


	void kill(int signal)
	{
		m_process.Signal(signal);
	}


private:
	int peekByte(unsigned long addr)
	{
		SBError error;
		uint8_t out;

		if (m_target.ReadMemory(SBAddress(addr, m_target), &out, sizeof(out), error) != sizeof(out))
			return -1;

		return out;
	}

	bool pokeByte(unsigned long addr, uint8_t data)
	{
		SBError error;

		return m_process.WriteMemory(addr, &data, sizeof(data), error) == sizeof(data);
	}

	unsigned long getAddress(const SBAddress &addr) const
	{
		if (m_useLoadAddresses)
			return addr.GetLoadAddress(m_target);

		return addr.GetFileAddress();
	}

	void handleModule(SBModule &module)
	{
		for (uint32_t i = 0; i < module.GetNumCompileUnits(); i++)
		{
			SBCompileUnit cu = module.GetCompileUnitAtIndex(i);

			if (!cu.IsValid())
				continue;

			handleCompileUnit(cu);
		}
	}

	// The file:line -> address mappings are in the compile units
	void handleCompileUnit(SBCompileUnit &cu)
	{
		for (uint32_t i = 0; i < cu.GetNumLineEntries(); i++)
		{
			SBLineEntry cur = cu.GetLineEntryAtIndex(i);

			if (!cur.IsValid())
				continue;

			// Filename
			SBFileSpec fs = cur.GetFileSpec();
			if (!fs.IsValid())
				continue;

			// The address where the program is at
			SBAddress addr = cur.GetStartAddress();

			std::string filename = fmt("%s/%s", fs.GetDirectory(), fs.GetFilename());

			std::string rp = m_filter->mangleSourcePath(filename);

			for (LineListenerList_t::const_iterator lit = m_lineListeners.begin();
				lit != m_lineListeners.end();
				++lit)
				(*lit)->onLine(rp, cur.GetLine(), getAddress(addr));
		}

	}

	typedef std::vector<ILineListener *> LineListenerList_t;
	typedef std::vector<IFileListener *> FileListenerList_t;
	typedef std::unordered_map<unsigned long, unsigned long > InstructionMap_t;

	bool m_useLoadAddresses;
	std::string m_filename;

	LineListenerList_t m_lineListeners;
	FileListenerList_t m_fileListeners;

	IEventListener *m_listener;
	SBDebugger m_debugger;
	SBTarget m_target;
	SBProcess m_process;
	SBBreakpoint m_forkBreakpoint;

	InstructionMap_t m_instructionMap;
	bool m_useLLDBBreakpoints;

	IFilter *m_filter;
};



// This ugly stuff was inherited from bashEngine
static LLDBEngine *g_lldbEngine;
class LLDBCtor
{
public:
	LLDBCtor()
	{
		g_lldbEngine = new LLDBEngine();
	}
};
static LLDBCtor g_bashCtor;


class LLDBEngineCreator : public IEngineFactory::IEngineCreator
{
public:
	virtual ~LLDBEngineCreator()
	{
	}

	virtual IEngine *create(IFileParser &parser)
	{
		return g_lldbEngine;
	}

	unsigned int matchFile(const std::string &filename, uint8_t *data, size_t dataSize)
	{
		// Unless #!/bin/sh etc, this should win
		return 1;
	}
};

static LLDBEngineCreator g_lldbEngineCreator;
