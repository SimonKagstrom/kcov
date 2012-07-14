#include <stdio.h>

int vobb(int a)
{
	printf("In shared library\n");

	return a + 1;
}
