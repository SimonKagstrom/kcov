#include <stdlib.h>

// Load this library and exit again!
void __attribute__((constructor)) meaningless_library()
{
	exit(93);
}
