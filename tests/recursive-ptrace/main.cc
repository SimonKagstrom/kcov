#include <unistd.h>
#include <stdio.h>
#include <sys/ptrace.h>
#include <sys/user.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sched.h>

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
		res = ptrace(PTRACE_TRACEME, 0, 0, 0);
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
	ptrace(PTRACE_SETOPTIONS, child, 0, PTRACE_O_TRACECLONE | PTRACE_O_TRACEFORK);

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
