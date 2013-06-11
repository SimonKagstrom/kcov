#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>

void daemonize(void)
{
	pid_t child;

	child = fork();

	if (child < 0) {
		perror("Fork failed?\n");
		exit(3);
	} else if (child == 0) {
		child = fork();

		if (child < 0) {
			perror("Fork failed?\n");
			exit(3);
		} else if (child > 0) {
			// Second parent
			exit(1);
		} else {
			freopen( "/dev/null", "r", stdin);
			freopen( "/dev/null", "w", stdout);
			freopen( "/dev/null", "w", stderr);

			// Daemonized child, just exit after a while
			sleep(5);
			exit(4);
		}
	} else {
		// First parent
		exit(2);
	}
}

int main(int argc, const char *argv[])
{
	daemonize();

	return 0;
}
