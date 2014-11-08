#include <stdio.h>

int vobb(int a)
{
	printf("In shared library\n");

	return a + 1;
}

void this_function_should_not_be_called(void)
{
	printf("If it is, something is wrong\n");
}
