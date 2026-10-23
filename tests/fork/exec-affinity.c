#include <stdlib.h>

// Prints how many CPUs a program started by this one may run on
int main(void)
{
	return system("nproc");
}
