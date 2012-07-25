#include <stdio.h>

static int first(int a)
{
	printf("first\n");
	return a + 1;
}

static int second(int a)
{
	printf("second\n");
	return a + 2;
}

int main(int argc, const char *argv[])
{
	if (argc == 1)
		first(argc);
	else
		second(argc);

	return 0;
}
