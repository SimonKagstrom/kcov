#include <unistd.h>
#include <stdio.h>
#include <sys/user.h>
#include <sys/types.h>
#include <sys/wait.h>

int main(int argc, const char *argv[])
{
	pid_t child;
	int status;

	child = vfork();
	if (child < 0) {
		fprintf(stderr, "vfork failed!\n");
		return -1;
	}

	if (child > 0)
		printf("Parent %d\n", getpid());
	else {
		execle("/bin/ls", "/bin/ls", "/proc/self/exe", NULL, NULL);
	}

	wait(&status);

	return 0;
}
