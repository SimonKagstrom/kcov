#include <sys/statvfs.h>
#include <sys/types.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

int test_popen(void)
{
	void *data = NULL;
	size_t chunk = 1024;
	size_t alloc_sz = chunk;
	size_t sz = 0;
	FILE *fp;

	fp = popen("/bin/ls -l /", "r");
	if (!fp)
	{
		fprintf(stderr, "popen failed\n");
		return 1;
	}

	/* Read in the entire file */
	while (!feof(fp)) {
		size_t ret;

		data = realloc(data, alloc_sz);
		if (!data)
		{
			fprintf(stderr, "realloc %u failed\n", alloc_sz);
			break;
		}

		ret = fread((uint8_t *)data + sz, 1, chunk, fp);
		sz += ret;
		/* Reached the end */
		if (ret < chunk)
			break;
		alloc_sz += chunk;
	}

	free(data);

	pclose(fp);

	return 0;
}

int main(int argc, const char *argv[])
{
	unsigned i;

	for (i = 0; i < 30; i++) {
		printf("Round %u\n", i);
		if (test_popen() != 0) {
			printf("FAIL\n");
			exit(0);
		}
	}
	printf("popen OK\n");

	return 0;
}
