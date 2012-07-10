#include <engine.hh>
#include <utils.hh>
#include <configuration.hh>
#include <output-handler.hh>
#include <elf.hh>
#include <phdr_data.h>

#include <unistd.h>
#include <sys/ptrace.h>
#include <sys/user.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>
#include <sched.h>
#include <map>
#include <unordered_map>
#include <list>

#include "library-binary.h"

using namespace kcov;

#define str(s) #s
#define xstr(s) str(s)

const char *solibPaths[] =
{
		xstr(KCOV_LIBRARY_PREFIX),
		"/usr/local/lib",
		"/opt/lib",
		"/usr/lib",
		"/lib",
		NULL
};

static unsigned long arch_getPcFromRegs(struct user_regs_struct *regs)
{
	unsigned long out;

#if defined(__i386__)
	out = regs->eip - 1;
#elif defined(__x86_64__)
	out = regs->rip - 1;
#else
# error Unsupported architecture
#endif

	return out;
}

static void arch_adjustPcAfterBreakpoint(struct user_regs_struct *regs)
{
#if defined(__i386__)
	regs->eip--;
#elif defined(__x86_64__)
	regs->rip--;
#else
# error Unsupported architecture
#endif
}

class Ptrace : public IEngine
{
public:
	Ptrace() : m_solibValid(false), m_solibFd(-1), m_ldPreloadString(NULL), m_envString(NULL)
	{
		m_breakpointId = 0;
		m_parentCpu = kcov_get_current_cpu();
		kcov_tie_process_to_cpu(getpid(), m_parentCpu);
	}

	bool readMemory(uint8_t *dst, unsigned long addr, size_t bytes)
	{
		unsigned long aligned = getAligned(addr);
		unsigned long offs = addr - aligned;
		unsigned long shift = 8 * offs;
		unsigned long data = ptrace(PTRACE_PEEKTEXT, m_activeChild, aligned, 0);

		panic_if(bytes != 1,
				"Can only read 1 byte now");

		*dst = (data & (0xffULL << shift)) >> shift;

		return true;
	}

	bool readProcessMemory(uint8_t *dst, void *start, size_t bytes)
	{
		panic_if (bytes != sizeof(unsigned long),
				"Can only read a word at a time now");

		panic_if ((unsigned long)start & (sizeof(unsigned long) - 1),
				"Address must be aligned");

		unsigned long data = ptrace(PTRACE_PEEKTEXT, m_activeChild, start, 0);
		memcpy(dst, &data, bytes);

		return true;
	}


	bool start(const char *executable)
	{
		m_breakpointToAddrMap.clear();
		m_addrToBreakpointMap.clear();
		m_instructionMap.clear();

		/* Basic check first */
		if (access(executable, X_OK) != 0)
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


		if (file_exists(kcov_solib_path.c_str()))
			putenv(m_ldPreloadString);
		putenv(m_envString);

		m_solibFd = open(kcov_solib_pipe_path.c_str(), O_RDONLY | O_NONBLOCK);

		unsigned int pid = IConfiguration::getInstance().getAttachPid();
		bool res = false;

		if (pid != 0)
			res = attachPid(pid);
		else
			res = forkChild(executable);

		return res;
	}

	int setBreakpoint(unsigned long addr)
	{
		uint8_t data;
		int id;

		// There already?
		if (m_addrToBreakpointMap.find(addr) != m_addrToBreakpointMap.end())
			return m_addrToBreakpointMap[addr];

		if (readMemory(&data, addr, 1) == false)
			return -1;

		id = m_breakpointId++;

		m_breakpointToAddrMap[id] = addr;
		m_addrToBreakpointMap[addr] = id;
		m_instructionMap[addr] = data;

		kcov_debug(BP_MSG, "BP set at 0x%lx\n", addr);

		// Set the breakpoint
		writeByte(m_activeChild, addr, 0xcc);

		return id;
	}

	void clearAllBreakpoints()
	{
		for (breakpointToAddrMap_t::iterator it = m_breakpointToAddrMap.begin();
				it != m_breakpointToAddrMap.end(); it++)
			clearBreakpoint(it->first);

		m_addrToBreakpointMap.clear();
		m_addrToBreakpointMap.clear();
		m_instructionMap.clear();
	}

	bool clearBreakpoint(int id)
	{
		if (m_breakpointToAddrMap.find(id) == m_breakpointToAddrMap.end())
			return false;

		unsigned long addr = m_breakpointToAddrMap[id];

		panic_if(m_addrToBreakpointMap.find(addr) == m_addrToBreakpointMap.end(),
				"Breakpoint id, but no addr-to-id map!");

		panic_if(m_instructionMap.find(addr) == m_instructionMap.end(),
				"Breakpoint found, but no instruction data at that point!");

		// Clear the actual breakpoint instruction
		writeByte(m_activeChild, addr, m_instructionMap[addr]);

		return true;
	}

	void singleStep()
	{
		struct user_regs_struct regs;

		ptrace(PTRACE_GETREGS, m_activeChild, 0, &regs);

		// Step back one instruction
		arch_adjustPcAfterBreakpoint(&regs);
		ptrace(PTRACE_SETREGS, m_activeChild, 0, &regs);
	}

	void checkSolibData()
	{
		if (m_solibValid)
			return;

		int r = read(m_solibFd, m_solibData, sizeof(m_solibData));

		if (r <= 0)
			return;
		panic_if ((unsigned)r >= sizeof(m_solibData),
				"Too much solib data read");

		struct phdr_data *p = phdr_data_unmarshal(m_solibData);

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

			IElf &elf = IElf::getInstance();

			elf.addFile(cur->name, cur);
			elf.parse();
		}

		m_solibValid = true;
	}

	const Event continueExecution()
	{
		Event out;
		int status;
		int who;
		int res;

		// Assume error
		out.type = ev_exit;
		out.data = -1;

		checkSolibData();

		kcov_debug(PTRACE_MSG, "PT continuing %d\n", m_activeChild);
		res = ptrace(PTRACE_CONT, m_activeChild, 0, 0);
		if (res < 0) {
			kcov_debug(PTRACE_MSG, "PT error for %d: %d\n", m_activeChild, res);
			return out;
		}

		while (1) {
			who = waitpid(-1, &status, __WALL);
			if (who == -1)
				return out;

			m_children[who] = 1;

			m_activeChild = who;
			out.addr = getPc(m_activeChild);

			kcov_debug(PTRACE_MSG, "PT stopped 0x%08x\n", status);

			// A signal?
			if (WIFSTOPPED(status)) {
				int sig = WSTOPSIG(status);

				// A trap?
				if (sig == SIGTRAP) {
					out.type = ev_breakpoint;
					out.data = -1;

					// Breakpoint id
					addrToBreakpointMap_t::iterator it = m_addrToBreakpointMap.find(out.addr);
					if (it != m_addrToBreakpointMap.end())
						out.data = it->second;

					kcov_debug(PTRACE_MSG, "PT BP at 0x%lx:%d\n", out.addr, out.data);
					if (out.data != -1)
						singleStep();
					return out;
				}
				else if (sig == SIGSEGV ||
						sig == SIGTERM ||
						sig == SIGFPE ||
						sig == SIGBUS ||
						sig == SIGILL) {
					out.type = ev_crash;
					out.data = sig;

					return out;
				} else if ((status >> 16) == PTRACE_EVENT_CLONE || (status >> 16) == PTRACE_EVENT_FORK) {
					printf("Hoolabaloo\n");
					sig = 0;
				} else if (sig == SIGSTOP) {
					out.type = ev_breakpoint;
					out.data = -1;
					return out;
				}

				// No, deliver it directly
				kcov_debug(PTRACE_MSG, "PT signal %d at 0x%lx for %d\n",
						WSTOPSIG(status), out.addr, m_activeChild);
				ptrace(PTRACE_CONT, m_activeChild, 0, sig);

				continue;
			}
			// Thread died?
			if (WIFSIGNALED(status) || WIFEXITED(status)) {
				m_children.erase(who);

				if (m_children.size() == 0) {
					Event tmp = continueExecution();
					if (tmp.type != ev_error)
						continue;

					out.type = ev_exit;
					out.data = WEXITSTATUS(status);
					return out;
				}

				continue;
			}

			out.type = ev_error;

			return out;
		}
	}

	std::string eventToName(Event ev)
	{
		switch (ev.type)
		{
		case ev_breakpoint:
			return std::string("breakpoint at 0x%lx", ev.addr);
		case ev_exit:
			return fmt("exit code %d", ev.data);
		case ev_crash:
		{
			if (ev.data == SIGSEGV)
				return std::string("SIGSEGV");
			if (ev.data == SIGILL)
				return std::string("SIGILL");
			if (ev.data == SIGTERM)
				return std::string("SIGTERM");
			if (ev.data == SIGBUS)
				return std::string("SIGBUS");
			if (ev.data == SIGFPE)
				return std::string("SIGFPE");

			return fmt("unknown signal %d", ev.data);
		}
		case ev_error:
			return std::string("error");
		default:
			break;
		}

		return std::string("unknown");
	}

	void kill()
	{
		ptrace(PTRACE_KILL, m_activeChild, 0, 0);
		ptrace(PTRACE_DETACH, m_activeChild, 0, 0);
	}

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
			execv(executable, argv);

			/* Exec failed */
			return false;
		}

		/* Fork error? */
		if (child < 0) {
			perror("fork");
			return false;
		}
		m_child = m_activeChild = child;
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
		m_activeChild = pid;

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

	unsigned long getPcFromRegs(struct user_regs_struct *regs)
	{
		return arch_getPcFromRegs(regs);
	}

	unsigned long getPc(int pid)
	{
		struct user_regs_struct regs;

		memset(&regs, 0, sizeof(regs));
		ptrace(PTRACE_GETREGS, pid, 0, &regs);

		return getPcFromRegs(&regs);
	}

	// Assume x86 with single-byte breakpoint instructions for now...
	void writeByte(int pid, unsigned long addr, uint8_t byte)
	{
		unsigned long aligned = getAligned(addr);
		unsigned long offs = addr - aligned;
		unsigned long shift = 8 * offs;
		unsigned long data = byte;
		unsigned long old_data;
		unsigned long val;

		old_data = ptrace(PTRACE_PEEKTEXT, pid, aligned, 0);
		val = (old_data & ~(0xffULL << shift)) | ((data & 0xff) << shift);
		ptrace(PTRACE_POKETEXT, pid, aligned, val);
	}

	unsigned long getAligned(unsigned long addr)
	{
		return (addr / sizeof(unsigned long)) * sizeof(unsigned long);
	}

	typedef std::unordered_map<int, unsigned long> breakpointToAddrMap_t;
	typedef std::unordered_map<unsigned long, int> addrToBreakpointMap_t;
	typedef std::unordered_map<unsigned long, uint8_t> instructionMap_t;
	typedef std::unordered_map<pid_t, int> ChildMap_t;

	int m_breakpointId;

	instructionMap_t m_instructionMap;
	breakpointToAddrMap_t m_breakpointToAddrMap;
	addrToBreakpointMap_t m_addrToBreakpointMap;

	pid_t m_activeChild;
	pid_t m_child;
	ChildMap_t m_children;

	bool m_solibValid;
	int m_solibFd;
	uint8_t m_solibData[128 * 1024];
	char *m_ldPreloadString;
	char *m_envString;

	int m_parentCpu;
};

IEngine &IEngine::getInstance()
{
	static Ptrace *instance;

	if (!instance)
		instance = new Ptrace();

	return *instance;
}
