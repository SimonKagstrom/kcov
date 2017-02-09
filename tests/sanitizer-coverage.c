#include <stdio.h>

static int fn(int a)
{
	printf("fn %d\n", a);

	a = a + 1;
	a = a + 2;

	return a + 3;
}

int main(int argc, const char *argv[])
{
	if (argc == 1) {
		int out = fn(argc + 1);

		out++;

		return out;
	} else {
		fn(5);
	}

	return 0;
}
