#!/usr/bin/env python3

import sys

def generate(idx, template):
	print("double fn%d(void) {" % (idx))
	print("%s" % (template))
	print("}")
	print("")

def generate_table(n):
	print("const unsigned n_functions = %d;" % (n))
	print("double (*functions[])(void) = {")
	for i in range(0, n):
		print("    fn%d," % (i))
	print("};")
	
if __name__ == "__main__":
	fileName = sys.argv[1]
	f = open(fileName)
	template = f.read()
	f.close()
	
	n = int(sys.argv[2])

	print("#include <math.h>")
	for i in range(0, n):
		generate(i, template)
	generate_table(n)
