// Small dummy test program to keep the addresses few!
int kalle(int a, int b)
{
	if (a == 15)
		return 1;

	// Should be two blocks
	if (a == 10 && b == 19)
		return 2;

	return 0;
}

void _start(int a, int b)
{
	if (kalle(a, b))
		return;

	kalle(9, 5);
}
