#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <signal.h>

int main(int argc, const char *argv[])
{
	pid_t child;

	child = fork();
	if (child < 0) {
		fprintf(stderr, "fork failed!\n");
		return -1;
	}

	if (child == 0) {
		printf("In child, waiting\n");
		sleep(5);
		printf("Wait done\n");
		return 0;
	}

	printf("Parent %d. Will crash\n", getpid());
	raise(SIGSEGV);

	return 0;
}
