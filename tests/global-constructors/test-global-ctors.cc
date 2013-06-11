#include <string>
#include <stdio.h>

std::string glob = "Kalle";

int main(int argc, const char *argv[])
{
	if (argc > 1)
		glob = "Manne";

	printf("XXX: %s\n", glob.c_str());

	return 0;
}
