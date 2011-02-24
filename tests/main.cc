#include <stdio.h>
#include "include/header.h"

class Test
{
public:
	Test()
	{
		printf("Constructor\n");
	}

	void hello()
	{
		printf("Hello\n");
	}
};

static Test g_test;

int main(int argc, const char *argv[])
{
	tjoho(1);
	tjoho2(0);

	return 0;
}
