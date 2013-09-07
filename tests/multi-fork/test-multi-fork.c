#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>

#include <sys/mman.h>

#include "multi-fork-generated.c"

int main(int argc, const char *argv[])
{
	size_t sz = 16 * 1024 * 1024;
	unsigned i;
	size_t n = n_functions;

	for (i = 0; i < n; i++) {
		pid_t child;

		child = fork();
		if (child < 0) {
			fprintf(stderr, "fork failed!\n");
			return 1;
		}

		if (child == 0) {
			// Child
			functions[i]();
			exit(0);
		}
	}

	while (1) {
		int status;
		int res;

		res = wait(&status);
		// No more children
		if (res == -1)
			break;
	}

	return 0;
}
