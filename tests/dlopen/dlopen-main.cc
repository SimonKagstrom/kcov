#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

extern void do_dlopen();

int main(int argc, const char *argv[])
{
	do_dlopen();
	sleep(1);

	return 0;
}
