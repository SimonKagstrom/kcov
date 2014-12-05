#include <engine.hh>
#include <utils.hh>
#include <configuration.hh>
#include <output-handler.hh>
#include <solib-handler.hh>
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
#include <map>
#include <unordered_map>
#include <list>
#include <mutex>
#include <vector>

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
		m_parentCpu(0),
		m_listener(NULL),
		m_signal(0),
		m_filter(NULL)
	{
		IEngineFactory::getInstance().registerEngine(*this);
	}

	~Ptrace()
	{
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

		m_instructionMap.clear();

		/* Basic check first */
		if (access(executable.c_str(), X_OK) != 0)
			return false;

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

		// There already?
		if (m_instructionMap.find(addr) != m_instructionMap.end())
			return 0;

		if (readMemory(&data, addr) == false)
			return -1;

		m_instructionMap[addr] = data;
		m_pendingBreakpoints.push_back(addr);

		kcov_debug(BP_MSG, "BP registered at 0x%lx\n", addr);

		return 0;
	}

	bool clearBreakpoint(unsigned long addr)
	{
		if (m_instructionMap.find(addr) == m_instructionMap.end()) {
			kcov_debug(BP_MSG, "Can't find breakpoint at 0x%lx\n", addr);

			return false;
		}

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

	const Event waitEvent()
	{
		static uint64_t lastSignalAddress;
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

			ptrace(PTRACE_GETSIGINFO, m_activeChild, NULL, (void *)&siginfo);

			// A trap?
			if (sig == SIGTRAP || sig == SIGSTOP) {
				out.type = ev_breakpoint;
				out.data = -1;

				kcov_debug(PTRACE_MSG, "PT BP at 0x%llx:%d for %d\n",
						(unsigned long long)out.addr, out.data, m_activeChild);
				// Single-step if we have this BP
				if (m_instructionMap.find(out.addr) != m_instructionMap.end())
					singleStep();
				else
					skipInstruction();

				if (m_firstBreakpoint) {
					blockUntilSolibDataRead();
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
			lastSignalAddress = out.addr;
		} else if (WIFSIGNALED(status)) {
			// Crashed/killed
			int sig = WTERMSIG(status);

			out.type = ev_signal;
			out.data = sig;
			out.addr = lastSignalAddress;

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

		setupAllBreakpoints();

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

		if (ev.type == ev_breakpoint)
			clearBreakpoint(ev.addr);

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

	void setupAllBreakpoints()
	{
		for (PendingBreakpointList_t::const_iterator addrIt = m_pendingBreakpoints.begin();
				addrIt != m_pendingBreakpoints.end();
				++addrIt) {
			unsigned long addr = *addrIt;
			unsigned long cur_data = peekWord(addr);

			// Set the breakpoint
			pokeWord(addr,	arch_setupBreakpoint(addr, cur_data));
		}

		m_pendingBreakpoints.clear();
	}


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

	// Skip over this instruction
	void skipInstruction()
	{
		// Nop on x86, op on PowerPC
#if defined(__powerpc__)
		unsigned long regs[1024];

		ptrace((__ptrace_request)PTRACE_GETREGS, m_activeChild, 0, &regs);

		regs[ppc_NIP] += 4;
		ptrace((__ptrace_request)PTRACE_SETREGS, m_activeChild, 0, &regs);
#endif
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

	typedef std::unordered_map<unsigned long, unsigned long > instructionMap_t;
	typedef std::vector<unsigned long> PendingBreakpointList_t;
	typedef std::unordered_map<pid_t, int> ChildMap_t;

	int m_breakpointId;

	instructionMap_t m_instructionMap;
	PendingBreakpointList_t m_pendingBreakpoints;
	bool m_firstBreakpoint;

	pid_t m_activeChild;
	pid_t m_child;
	pid_t m_firstChild;
	ChildMap_t m_children;

	int m_parentCpu;

	IEventListener *m_listener;
	unsigned long m_signal;

	IFilter *m_filter;
};

static Ptrace::Ctor g_ptraceEngine;
