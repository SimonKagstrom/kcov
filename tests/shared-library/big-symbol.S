.text
.globl very_big_symbol
very_big_symbol:
	.fill 4096, 1, 0x90
#if defined(__PPC__)
	blr
#else
	ret
#endif
