#include <file-parser.hh>
#include <engine.hh>
#include <configuration.hh>
#include <output-handler.hh>
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
		m_listener(NULL)
	{
		SBDebugger::Initialize();
		m_debugger = SBDebugger::Create();

		m_debugger.SetAsync(false);
		IParserManager::getInstance().registerParser(*this);
	}

	virtual ~LLDBEngine()
	{
	}

	// From IEngine
	virtual int registerBreakpoint(unsigned long addr)
	{
		SBBreakpoint bp = m_target.BreakpointCreateByAddress(addr);

		if (!bp.IsValid())
			return -1;

		return bp.GetID();
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
		// Handled when the program is launched
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

		m_listener = &listener;

		if (getcwd(buf, sizeof(buf)) < 0) {
			error("No current WD?\n");

			return false;
		}

		unsigned int pid = IConfiguration::getInstance().keyAsInt("attach-pid");

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

		// Send all signals to the process
		SBUnixSignals sigs = m_process.GetUnixSignals();
		for (unsigned int i = SIGHUP; i < SIGUSR2; i++) {
			sigs.SetShouldNotify(i, false);
			sigs.SetShouldStop(i, true);
			sigs.SetShouldSuppress(i, false);
		}

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

	bool checkEvents()
	{
		return false;
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

				m_target.BreakpointDelete(id);
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

			for (LineListenerList_t::const_iterator lit = m_lineListeners.begin();
				lit != m_lineListeners.end();
				++lit)
				(*lit)->onLine(filename, cur.GetLine(), getAddress(addr));
		}

	}

	void reportEvent(enum event_type type, int data = -1, uint64_t address = 0)
	{
		if (!m_listener)
			return;

		m_listener->onEvent(Event(type, data, address));
	}


	typedef std::vector<ILineListener *> LineListenerList_t;
	typedef std::vector<IFileListener *> FileListenerList_t;

	bool m_useLoadAddresses;
	std::string m_filename;

	LineListenerList_t m_lineListeners;
	FileListenerList_t m_fileListeners;

	IEventListener *m_listener;
	SBDebugger m_debugger;
	SBTarget m_target;
	SBProcess m_process;
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
