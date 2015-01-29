#include <stdio.h>

int vobb(int a)
{
	printf("In shared library\n");

	a = a + 1;

	if (a > 2)
		return 1;

	return 0;
}

void this_function_should_not_be_called(void)
{
	printf("If it is, something is wrong\n");
}
