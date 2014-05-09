#include <stdio.h>

extern int mibb(void);

int main(int argc, const char *argv[])
{
	int v = 0;

	if (argc > 1)
		v = mibb();

	printf("Second: V is %d\n", v);
}
