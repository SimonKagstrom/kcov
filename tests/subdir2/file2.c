#include <stdio.h>
#include "../include/header.h"

int b;
void tjoho2(int a)
{
	int i;
	for (i = 0; i < 2; i++)
		b++;
	a++;

	if (a)
		printf("Hej\n");
	if (a + 1 == 19)
		printf("Svampo\n");
}
