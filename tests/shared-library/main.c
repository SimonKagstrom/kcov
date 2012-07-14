#include <stdio.h>

extern int vobb(int a);

int main(int argc, const char *argv[])
{
	printf("In main\n");

	vobb(argc);

	return 0;
}
