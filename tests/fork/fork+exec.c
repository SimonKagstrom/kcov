#include <unistd.h>
#include <stdio.h>
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

	if (child > 0) {
		printf("Parent %d\n", getpid());
	} else if (child < 0) {
		printf("ERROR!\n");
	} else {
		printf("Child, will exec()\n");
		execvp(argv[1], NULL);
	}

	waitpid(child, &status, 0);

	return 0;
}
