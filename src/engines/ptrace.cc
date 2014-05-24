#include <engine.hh>
#include <utils.hh>
#include <configuration.hh>
#include <output-handler.hh>
#include <file-parser.hh>
#include <phdr_data.h>

#include <unistd.h>
#include <sys/ptrace.h>
#include <sys/user.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <libelf.h>
#include <signal.h>
#include <fcntl.h>
#include <sched.h>
#include <pthread.h>
#include <map>
#include <unordered_map>
#include <list>
#include <mutex>

#include "../library-binary.h"

using namespace kcov;

#define str(s) #s
#define xstr(s) str(s)

enum
{
	i386_EIP = 12,
	x86_64_RIP = 16,
	ppc_NIP = 32,
	arm_PC = 15,
};

static unsigned long getAligned(unsigned long addr)
{
	return (addr / sizeof(unsigned long)) * sizeof(unsigned long);
}

static unsigned long arch_getPcFromRegs(unsigned long *regs)
{
	unsigned long out;

#if defined(__i386__)
	out = regs[i386_EIP] - 1;
#elif defined(__x86_64__)
	out = regs[x86_64_RIP] - 1;
#elif defined(__arm__)
	out = regs[arm_PC] - 4;
#elif defined(__powerpc__)
	out = regs[ppc_NIP];
#else
# error Unsupported architecture
#endif

	return out;
}

static void arch_adjustPcAfterBreakpoint(unsigned long *regs)
{
#if defined(__i386__)
	regs[i386_EIP]--;
#elif defined(__x86_64__)
	regs[x86_64_RIP]--;
#elif defined(__arm__)
	regs[arm_PC] -= 4;
#elif defined(__powerpc__)
	// Do nothing
#else
# error Unsupported architecture
#endif
}


static unsigned long arch_setupBreakpoint(unsigned long addr, unsigned long old_data)
{
	unsigned long val;

#if defined(__i386__) || defined(__x86_64__)
	unsigned long aligned_addr = getAligned(addr);
	unsigned long offs = addr - aligned_addr;
	unsigned long shift = 8 * offs;

	val = (old_data & ~(0xffUL << shift)) |
			(0xccUL << shift);
#elif defined(__powerpc__)
	val =  0x7fe00008; /* tw */
#elif defined(__arm__)
	val = 0x123456; //0xe1200070; /* BKPT */
#else
# error Unsupported architecture
#endif

	return val;
}

static unsigned long arch_clearBreakpoint(unsigned long addr, unsigned long old_data, unsigned long cur_data)
{
	unsigned long val;
#if defined(__i386__) || defined(__x86_64__)
	unsigned long aligned_addr = getAligned(addr);
	unsigned long offs = addr - aligned_addr;
	unsigned long shift = 8 * offs;
	unsigned long old_byte = (old_data >> shift) & 0xffUL;

	val = (cur_data & ~(0xffUL << shift)) |
			(old_byte << shift);
#elif defined(__powerpc__) || defined(__arm__)
	val = old_data;
#else
# error Unsupported architecture
#endif

	return val;
}



class Ptrace : public IEngine
{
public:
	Ptrace() :
		m_breakpointId(0),
		m_firstBreakpoint(true),
		m_activeChild(0),
		m_child(0),
		m_firstChild(0),
		m_ldPreloadString(nullptr),
		m_envString(nullptr),
		m_parentCpu(0),
		m_solibFileOpen(false),
		m_elf(NULL),
		m_listener(NULL),
		m_signal(0)
	{
		memset(&m_solibThread, 0, sizeof(m_solibThread));

		IEngineFactory::getInstance().registerEngine(*this);
	}

	~Ptrace()
	{
		void *rv;

		unlink(m_solibPath.c_str());
		close(m_solibFd);

		/*
		 * First kill the solib thread if it's stuck in open (i.e., before
		 * the tracee has started), then wait for it to terminate for maximum
		 * niceness.
		 */
		pthread_kill(m_solibThread, SIGTERM);
		pthread_join(m_solibThread, &rv);
		kill(SIGTERM);
		ptrace(PTRACE_DETACH, m_activeChild, 0, 0);
	}


	bool readMemory(unsigned long *dst, unsigned long addr)
	{
		*dst = peekWord(addr);

		return true;
	}



	bool start(IEventListener &listener, const std::string &executable)
	{
		m_listener = &listener;

		m_parentCpu = kcov_get_current_cpu();
		kcov_tie_process_to_cpu(getpid(), m_parentCpu);

		m_breakpointToAddrMap.clear();
		m_addrToBreakpointMap.clear();
		m_instructionMap.clear();

		/* Basic check first */
		if (access(executable.c_str(), X_OK) != 0)
			return false;

		std::string kcov_solib_pipe_path =
				IOutputHandler::getInstance().getOutDirectory() +
				"kcov-solib.pipe";
		std::string kcov_solib_path =
				IOutputHandler::getInstance().getBaseDirectory() +
				"libkcov_sowrapper.so";

		write_file(__library_data, __library_data_size, kcov_solib_path.c_str());

		std::string kcov_solib_env = "KCOV_SOLIB_PATH=" +
				kcov_solib_pipe_path;
		unlink(kcov_solib_pipe_path.c_str());
		mkfifo(kcov_solib_pipe_path.c_str(), 0644);

		free(m_envString);
		m_envString = (char *)xmalloc(kcov_solib_env.size() + 1);
		strcpy(m_envString, kcov_solib_env.c_str());

		std::string preloadEnv = std::string("LD_PRELOAD=" + kcov_solib_path).c_str();
		free(m_ldPreloadString);
		m_ldPreloadString = (char *)xmalloc(preloadEnv.size() + 1);
		strcpy(m_ldPreloadString, preloadEnv.c_str());

		if (IConfiguration::getInstance().getParseSolibs()) {
			if (file_exists(kcov_solib_path.c_str()))
				putenv(m_ldPreloadString);
		}
		putenv(m_envString);

		m_solibPath = kcov_solib_pipe_path;
		pthread_create(&m_solibThread, nullptr,
				Ptrace::threadStatic, (void *)this);

		unsigned int pid = IConfiguration::getInstance().getAttachPid();
		bool res = false;

		if (pid != 0)
			res = attachPid(pid);
		else
			res = forkChild(executable.c_str());

		return res;
	}

	int registerBreakpoint(unsigned long addr)
	{
		unsigned long data;
		int id;

		// There already?
		if (m_addrToBreakpointMap.find(addr) != m_addrToBreakpointMap.end())
			return m_addrToBreakpointMap[addr];

		if (readMemory(&data, addr) == false)
			return -1;

		id = m_breakpointId++;

		m_breakpointToAddrMap[id] = addr;
		m_addrToBreakpointMap[addr] = id;
		m_instructionMap[addr] = data;
		m_pendingBreakpoints.push_back(addr);

		kcov_debug(BP_MSG, "BP registered at 0x%lx\n", addr);

		return id;
	}

	void setupAllBreakpoints()
	{
		for (const auto addr : m_pendingBreakpoints) {
			unsigned long cur_data = peekWord(addr);

			// Set the breakpoint
			pokeWord(addr,	arch_setupBreakpoint(addr, cur_data));
		}

		m_pendingBreakpoints.clear();
	}

	void solibThreadMain()
	{
		uint8_t buf[1024 * 1024];

		m_solibFd = open(m_solibPath.c_str(), O_RDONLY);

		if (m_solibFd < 0)
			return;

		m_solibFileOpen = true;

		while (1) {
			int r = read(m_solibFd, buf, sizeof(buf));

			// The destructor will close m_solibFd, so we'll exit here in that case
			if (r <= 0)
				break;

			panic_if ((unsigned)r >= sizeof(buf),
					"Too much solib data read");

			struct phdr_data *p = phdr_data_unmarshal(buf);

			if (p) {
				size_t sz = sizeof(struct phdr_data) + p->n_entries * sizeof(struct phdr_data_entry);
				struct phdr_data *cpy = (struct phdr_data*)xmalloc(sz);

				memcpy(cpy, p, sz);

				m_phdrListMutex.lock();
				m_phdrs.push_back(cpy);
				m_phdrListMutex.unlock();
			}
			m_solibDataReadSemaphore.notify();
		}

		m_solibDataReadSemaphore.notify();
		close(m_solibFd);
	}

	static void *threadStatic(void *pThis)
	{
		Ptrace *p = (Ptrace *)pThis;

		p->solibThreadMain();

		// Never reached
		return nullptr;
	}

	void clearAllBreakpoints()
	{
		for (const auto &it : m_breakpointToAddrMap)
			clearBreakpoint(it.first);

		m_addrToBreakpointMap.clear();
		m_addrToBreakpointMap.clear();
		m_instructionMap.clear();
	}

	bool clearBreakpoint(int id)
	{
		if (m_breakpointToAddrMap.find(id) == m_breakpointToAddrMap.end()) {
			kcov_debug(BP_MSG, "Can't find breakpoint %d\n", id);
			return false;
		}

		unsigned long addr = m_breakpointToAddrMap[id];

		panic_if(m_addrToBreakpointMap.find(addr) == m_addrToBreakpointMap.end(),
				"Breakpoint id, but no addr-to-id map!");

		panic_if(m_instructionMap.find(addr) == m_instructionMap.end(),
				"Breakpoint found, but no instruction data at that point!");


		// Clear the actual breakpoint instruction
		unsigned long val = m_instructionMap[addr];
		val = arch_clearBreakpoint(addr, val, peekWord(addr));

		pokeWord(addr, val);

		return true;
	}

	void singleStep()
	{
		unsigned long regs[1024];

		ptrace((__ptrace_request)PTRACE_GETREGS, m_activeChild, 0, &regs);

		// Step back one instruction
		arch_adjustPcAfterBreakpoint(regs);
		ptrace((__ptrace_request)PTRACE_SETREGS, m_activeChild, 0, &regs);
	}

	void checkSolibData()
	{
		struct phdr_data *p = nullptr;

		if (!m_elf)
			return;

		m_phdrListMutex.lock();
		if (!m_phdrs.empty()) {
			p = m_phdrs.front();
			m_phdrs.pop_front();
		}
		m_phdrListMutex.unlock();

		if (!p)
			return;

		for (unsigned int i = 0; i < p->n_entries; i++)
		{
			struct phdr_data_entry *cur = &p->entries[i];

			if (strlen(cur->name) == 0)
				continue;

			// Skip this very special library
			if (strstr(cur->name, "libkcov_sowrapper.so"))
				continue;

			m_elf->addFile(cur->name, cur);
			m_elf->parse();

			setupAllBreakpoints();
		}

		free(p);
	}

	const Event waitEvent()
	{
		Event out;
		int status;
		int who;

		// Assume error
		out.type = ev_error;
		out.data = -1;

		who = waitpid(-1, &status, __WALL);
		if (who == -1) {
			kcov_debug(PTRACE_MSG, "Returning error\n");
			return out;
		}

		m_children[who] = 1;

		m_activeChild = who;
		out.addr = getPc(m_activeChild);

		kcov_debug(PTRACE_MSG, "PT stopped PID %d 0x%08x\n", m_activeChild, status);

		// A signal?
		if (WIFSTOPPED(status)) {
			siginfo_t siginfo;
			int sig = WSTOPSIG(status);

			out.type = ev_signal;
			out.data = sig;

			ptrace(PTRACE_GETSIGINFO, m_activeChild, nullptr, (void *)&siginfo);

			// A trap?
			if (sig == SIGTRAP || sig == SIGSTOP) {
				out.type = ev_breakpoint;
				out.data = -1;

				// Breakpoint id
				addrToBreakpointMap_t::iterator it = m_addrToBreakpointMap.find(out.addr);
				if (it != m_addrToBreakpointMap.end())
					out.data = it->second;

				kcov_debug(PTRACE_MSG, "PT BP at 0x%llx:%d for %d\n",
						(unsigned long long)out.addr, out.data, m_activeChild);
				if (out.data != -1)
					singleStep();

				if (m_solibFileOpen && m_firstBreakpoint) {
					m_solibDataReadSemaphore.wait();
					m_firstBreakpoint = false;
				}

				return out;
			} else if ((status >> 16) == PTRACE_EVENT_CLONE || (status >> 16) == PTRACE_EVENT_FORK) {
				kcov_debug(PTRACE_MSG, "PT clone at 0x%llx for %d\n",
						(unsigned long long)out.addr, m_activeChild);
				out.data = 0;
			}

			kcov_debug(PTRACE_MSG, "PT signal %d at 0x%llx for %d\n",
					WSTOPSIG(status), (unsigned long long)out.addr, m_activeChild);
		} else if (WIFSIGNALED(status)) {
			// Crashed/killed
			int sig = WTERMSIG(status);

			out.type = ev_signal;
			out.data = sig;

			kcov_debug(PTRACE_MSG, "PT terminating signal %d at 0x%llx for %d\n",
					sig, (unsigned long long)out.addr, m_activeChild);
			m_children.erase(who);

			if (!childrenLeft())
				out.type = ev_signal_exit;

		} else if (WIFEXITED(status)) {
			int exitStatus = WEXITSTATUS(status);

			kcov_debug(PTRACE_MSG, "PT exit %d at 0x%llx for %d%s\n",
					exitStatus, (unsigned long long)out.addr, m_activeChild, m_activeChild == m_firstChild ? " (first child)" : "");

			m_children.erase(who);

			if (who == m_firstChild)
				out.type = ev_exit_first_process;
			else
				out.type = ev_exit;

			out.data = exitStatus;
		}

		return out;
	}

	bool childrenLeft()
	{
		return m_children.size() > 0 &&
				 m_children.find(m_firstChild) != m_children.end();
	}


		/**
		 * Continue execution with an event
		 */
	bool continueExecution()
	{
		int res;

		checkSolibData();

		kcov_debug(PTRACE_MSG, "PT continuing %d with signal %lu\n", m_activeChild, m_signal);
		res = ptrace(PTRACE_CONT, m_activeChild, 0, m_signal);
		if (res < 0) {
			kcov_debug(PTRACE_MSG, "PT error for %d: %d\n", m_activeChild, res);
			m_children.erase(m_activeChild);
		}


		Event ev = waitEvent();
		m_signal = ev.type == ev_signal ? ev.data : 0;

		if (m_listener)
			m_listener->onEvent(ev);

		if (ev.type == ev_error)
			return false;

		return true;
	}

	void kill(int signal)
	{
		// Don't kill kcov itself (PID 0)
		if (m_activeChild != 0)
			::kill(m_activeChild, signal);
	}

	unsigned int matchFile(const std::string &filename, uint8_t *data, size_t dataSize)
	{
		m_elf = IParserManager::getInstance().matchParser(filename);

		// We need a parser for this to work
		if (!m_elf)
			return match_none;

		// Unless #!/bin/sh etc, this should win
		return 1;
	}

	class Ctor
	{
	public:
		Ctor()
		{
			new Ptrace();
		}
	};

private:
	bool forkChild(const char *executable)
	{
		char *const *argv = (char *const *)IConfiguration::getInstance().getArgv();
		pid_t child, who;
		int status;

		/* Executable exists, try to launch it */
		if ((child = fork()) == 0) {
			int res;

			/* And launch the process */
			res = ptrace(PTRACE_TRACEME, 0, 0, 0);
			if (res < 0) {
				perror("Can't set me as ptraced");
				return false;
			}
			kcov_tie_process_to_cpu(getpid(), m_parentCpu);
			execv(executable, argv);

			/* Exec failed */
			return false;
		}

		/* Fork error? */
		if (child < 0) {
			perror("fork");
			return false;
		}
		m_child = m_activeChild = m_firstChild = child;
		// Might not be completely necessary (the child should inherit this
		// from the parent), but better safe than sorry
		kcov_tie_process_to_cpu(m_child, m_parentCpu);

		kcov_debug(PTRACE_MSG, "PT forked %d\n", child);

		/* Wait for the initial stop */
		who = waitpid(child, &status, 0);
		if (who < 0) {
			perror("waitpid");
			return false;
		}
		if (!WIFSTOPPED(status)) {
			fprintf(stderr, "Child hasn't stopped: %x\n", status);
			return false;
		}

		ptrace(PTRACE_SETOPTIONS, m_activeChild, 0, PTRACE_O_TRACECLONE | PTRACE_O_TRACEFORK);

		return true;
	}

	bool attachPid(pid_t pid)
	{
		m_child = m_activeChild = m_firstChild = pid;

		errno = 0;
		ptrace(PTRACE_ATTACH, m_activeChild, 0, 0);
		if (errno) {
			const char *err = strerror(errno);

			fprintf(stderr, "Can't attach to %d. Error %s\n", pid, err);
			return false;
		}

		/* Wait for the initial stop */
		int status;
		int who = waitpid(m_activeChild, &status, 0);
		if (who < 0) {
			perror("waitpid");
			return false;
		}
		if (!WIFSTOPPED(status)) {
			fprintf(stderr, "Child hasn't stopped: %x\n", status);
			return false;
		}
		kcov_tie_process_to_cpu(m_activeChild, m_parentCpu);

		::kill(m_activeChild, SIGSTOP);
		ptrace(PTRACE_SETOPTIONS, m_activeChild, 0, PTRACE_O_TRACECLONE | PTRACE_O_TRACEFORK);

		return true;
	}

	unsigned long getPcFromRegs(unsigned long *regs)
	{
		return arch_getPcFromRegs(regs);
	}

	unsigned long getPc(int pid)
	{
		unsigned long regs[1024];

		memset(&regs, 0, sizeof(regs));
		ptrace((__ptrace_request)PTRACE_GETREGS, pid, 0, &regs);

		return getPcFromRegs(regs);
	}

	unsigned long peekWord(unsigned long addr)
	{
		unsigned long aligned = getAligned(addr);

		return ptrace((__ptrace_request)PTRACE_PEEKTEXT, m_activeChild, aligned, 0);
	}

	void pokeWord(unsigned long addr, unsigned long val)
	{
		ptrace((__ptrace_request)PTRACE_POKETEXT, m_activeChild, getAligned(addr), val);
	}

	typedef std::unordered_map<int, unsigned long> breakpointToAddrMap_t;
	typedef std::unordered_map<unsigned long, int> addrToBreakpointMap_t;
	typedef std::unordered_map<unsigned long, unsigned long > instructionMap_t;
	typedef std::list<unsigned long> PendingBreakpointList_t;
	typedef std::unordered_map<pid_t, int> ChildMap_t;
	typedef std::list<struct phdr_data *> PhdrList_t;

	int m_breakpointId;

	instructionMap_t m_instructionMap;
	breakpointToAddrMap_t m_breakpointToAddrMap;
	addrToBreakpointMap_t m_addrToBreakpointMap;
	PendingBreakpointList_t m_pendingBreakpoints;
	PhdrList_t m_phdrs;
	std::mutex m_phdrListMutex;
	pthread_t m_solibThread;
	Semaphore m_solibDataReadSemaphore;
	bool m_firstBreakpoint;

	pid_t m_activeChild;
	pid_t m_child;
	pid_t m_firstChild;
	ChildMap_t m_children;

	std::string m_solibPath;
	char *m_ldPreloadString;
	char *m_envString;

	int m_parentCpu;

	bool m_solibFileOpen;
	int m_solibFd;

	IFileParser *m_elf;
	IEventListener *m_listener;
	unsigned long m_signal;
};

static Ptrace::Ctor g_ptraceEngine;
