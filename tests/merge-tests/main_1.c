#include <stdio.h>

extern int vobb(void);

int main(int argc, const char *argv[])
{
	int v = 0;

	if (argc > 1)
		v = vobb();

	printf("First: V is %d\n", v);
}
