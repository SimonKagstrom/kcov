#include <sys/types.h>
#include <sys/ptrace.h>
#include <sys/user.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>
#include <sched.h>
#include <stdio.h>

int forkAndAttach()
{
	pid_t child, who;
	int status;

	child = fork();
	if (child < 0) {
		fprintf(stderr, "fork failed!\n");
		return -1;
	}

	if (child == 0) {
		int res;

		/* We're in the child, set me as traced */
#ifdef __linux__
		res = ptrace(PTRACE_TRACEME, 0, 0, 0);
#elif defined(__FreeBSD__)
		res = ptrace(PT_TRACE_ME, 0, 0, 0);
#endif
		if (res < 0) {
			fprintf(stderr, "Can't set me as ptraced\n");
			return -1;
		}
		::kill(getpid(), SIGSTOP);

		return 0;
	}

	/* Wait for the initial stop */
	who = waitpid(child, &status, 0);
	if (who < 0) {
		fprintf(stderr, "waitpid failed\n");
		return -1;
	}
	if (!WIFSTOPPED(status)) {
		fprintf(stderr, "Child hasn't stopped: %x\n", status);
		return -1;
	}
#ifdef __linux__
	ptrace(PTRACE_SETOPTIONS, child, 0, PTRACE_O_TRACECLONE | PTRACE_O_TRACEFORK);
#elif defined(__FreeBSD__)
	ptrace(PT_FOLLOW_FORK, child, NULL, 1);
#endif

	return child;
}

int main(int argc, const char *argv[])
{
	int res = forkAndAttach();

	if (res >= 0)
		printf("OK!\n");
	else
		printf("NOK!\n");

	return res >= 0;
}
