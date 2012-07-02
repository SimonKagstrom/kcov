#include <unistd.h>
#include <stdio.h>
#include <sys/ptrace.h>
#include <sys/user.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sched.h>

int main(int argc, const char *argv[])
{
	pid_t child;

	child = fork();
	if (child < 0) {
		fprintf(stderr, "fork failed!\n");
		return -1;
	}

	if (child > 0)
		printf("Parent %d. Will exit\n", getpid());
	else {
		printf("In child, waiting\n");
		sleep(1);
		printf("Wait done, parent should be finished by now\n");
	}

	/* No wait for children */

	return 0;
}
