/*
 * Copyright (C) 2010 Simon Kagstrom, Thomas Neumann
 *
 * See COPYING for license details
 *
 * Taken from bcov:Debugger.cpp:
 *
 * Copyright (C) 2007 Thomas Neumann
 */
#include <sys/ptrace.h>
#include <sys/user.h>
#include <sys/wait.h>
#include <sys/errno.h>

#include <kc.h>
#include <utils.h>

static pid_t active_child, child;

static unsigned char peek_byte(const void *ptr)
{
	unsigned long addr = (unsigned long)ptr;
	unsigned long aligned = (addr / sizeof(long)) * sizeof(long);
	union { long val; unsigned char data[sizeof(long)]; } data;

	data.val = ptrace(PTRACE_PEEKTEXT, active_child, aligned, 0);

	return data.data[addr - aligned];
}

static void poke_byte(unsigned long addr, unsigned char c)
{
	unsigned long aligned = (addr / sizeof(long)) * sizeof(long);
	union { long val; unsigned char data[sizeof(long)]; } data;

	data.val = ptrace(PTRACE_PEEKTEXT, active_child, aligned, 0);
	data.data[addr - aligned] = c;

	ptrace(PTRACE_POKETEXT, active_child, aligned, data.val);
}

static void *ptrace_get_ip(void)
{
   struct user_regs_struct regs;

   memset(&regs, 0, sizeof(regs));
   ptrace(PTRACE_GETREGS, active_child, 0, &regs);

#if defined(__x86_64__)
   return (void*)(regs.rip);
#elif defined(__i386__)
   return (void*)(regs.eip);
#else
   #error specify how to read the IP
#endif
}

void* ptrace_get_ip_before_trap(void)
{
#if defined(__x86_64__) || defined(__i386__)
   return (char*)(ptrace_get_ip()) - 1;
#else
   #error specify how to adjust the IP after a breakpoint
#endif
}

static void ptrace_setup_breakpoints(struct kc *kc)
{
	GHashTableIter iter;
	unsigned long key;
	struct kc_addr *addr;

	g_hash_table_iter_init(&iter, kc->addrs);
	while (g_hash_table_iter_next(&iter,
			(gpointer*)&key, (gpointer*)&addr)) {
		uint8_t old_byte = peek_byte((void *)addr->addr);

		addr->saved_code = old_byte;
#if defined(__x86_64__)||defined(__i386__)
      poke_byte(addr->addr, 0xCC);
#else
   #error specify how to set a breakpoint
#endif
	}
}

void ptrace_eliminate_breakpoint(struct kc_addr *addr)
{
   struct user_regs_struct regs;
   memset(&regs, 0, sizeof(regs));
   unsigned long ptr;

   ptrace(PTRACE_GETREGS, active_child, 0, &regs);
#if defined(__x86_64__)
   ptr = (unsigned long)(--regs.rip);
#elif defined(__i386__)
   ptr = (unsigned long)(--regs.eip);
#else
   #error specify how to adjust the IP after a breakpoint
#endif
   ptrace(PTRACE_SETREGS, active_child, 0, &regs);

   poke_byte(ptr, addr->saved_code);
   kc_addr_register_hit(addr);
}

enum {
	PT_CODE_ERROR,
	PT_CODE_TRAP,
	PT_CODE_EXIT,
};

static int do_ptrace_run(struct kc *kc)
{
	// Continue the stopped child
	ptrace(PTRACE_CONT, active_child, 0, 0);

	while (1) {
		// Wait for a child
		int status;

		pid_t r = waitpid(-1, &status, __WALL);

		// Got no one? Child probably died
		if (r == -1)
			return PT_CODE_EXIT;
		active_child = r;

		// A signal?
		if (WIFSTOPPED(status)) {
			// A trap?
			if (WSTOPSIG(status) == SIGTRAP)
				return PT_CODE_TRAP;
			// No, deliver it directly
			ptrace(PTRACE_CONT, active_child, 0, WSTOPSIG(status));
			continue;
		}
		// Thread died?
		if (WIFSIGNALED(status) || WIFEXITED(status)) {
			if (active_child == child)
				return PT_CODE_EXIT;
			continue;
		}
		// Stopped?
		if (WIFSTOPPED(status)) {
			// A new clone? Ignore the stop event
			if ((status >> 8) == PTRACE_EVENT_CLONE) {
				ptrace(PTRACE_CONT, active_child, 0, 0);
				continue;
			}
			// Hm, why did we stop? Ignore the event and continue
			ptrace(PTRACE_CONT, active_child, 0, 0);
			continue;
		}

		// Unknown event
		return PT_CODE_ERROR;
	}
}

static void ptrace_run_debugger(struct kc *kc)
{
	while (1) {
		int err = do_ptrace_run(kc);

		switch (err) {
		case PT_CODE_ERROR:
			fprintf(stderr, "Error while tracing\n");
		case PT_CODE_EXIT:
			return;
		case PT_CODE_TRAP:
		{
			void *where = ptrace_get_ip_before_trap();
			struct kc_addr *addr = kc_lookup_addr(kc, (unsigned long)where);

			if (addr)
				ptrace_eliminate_breakpoint(addr);

		} break;
      }
   }
}

static pid_t fork_child(const char *executable, char *const argv[])
{
	/* Basic check first */
	if (access(executable, X_OK) != 0)
		return -1;

	/* Executable exists, try to launch it */
	if ((child = fork()) == 0) {

		/* And launch the process */
		ptrace(PTRACE_TRACEME, 0, 0, 0);
		execv(executable, argv);

		/* Exec failed */
		return -1;
	}

	/* Fork error? */
	if (child < 0)
		return -1;

	/* Wait for the initial stop */
	int status;
	if ((waitpid(child, &status, 0) == -1) || (!WIFSTOPPED(status)))
		return -1;
	ptrace(PTRACE_SETOPTIONS, child, 0, PTRACE_O_TRACECLONE | PTRACE_O_TRACEFORK);

	return child;
}



int ptrace_run(struct kc *kc, char *const argv[])
{
	active_child = fork_child(argv[0], &argv[0]);

	if (active_child < 0) {
		fprintf(stderr, "Can't fork child!\n");
		return -1;
	}

	ptrace_setup_breakpoints(kc);

	ptrace_run_debugger(kc);

	return 0;
}

int ptrace_pid_run(struct kc *kc, pid_t pid)
{
	active_child = child = pid;

	errno = 0;
	ptrace(PTRACE_ATTACH, active_child, 0, 0);
	if (errno) {
		const char *err = strerror(errno);

		fprintf(stderr, "Can't attach to %d. Error %s\n", pid, err);
		return -1;
	}
	ptrace(PTRACE_SETOPTIONS, child, 0, PTRACE_O_TRACECLONE | PTRACE_O_TRACEFORK);

	ptrace_setup_breakpoints(kc);

	ptrace_run_debugger(kc);

	return 0;
}
