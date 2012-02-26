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
	int status;

	child = fork();
	if (child < 0) {
		fprintf(stderr, "fork failed!\n");
		return -1;
	}

	if (child == 0)
		printf("Parent %d\n", getpid());
	else {
		pid_t grand_child = fork();


		if (grand_child < 0)
			fprintf(stderr, "fork gc failed\n");
		else if (grand_child == 0)
			printf("Grand child %d\n", getpid());
		else
			printf("Child %d\n", getpid());
		wait(&status);
	}

	printf("Waiting in %d\n", getpid());
	wait(&status);

	return 0;
}
