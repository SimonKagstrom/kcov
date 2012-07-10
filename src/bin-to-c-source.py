#!/usr/bin/env python

import sys, struct

def generate(data_in):
	print "#include <stdint.h>"
	print "#include <stdlib.h>"

	print "size_t __library_data_size = 0x%x;" % (len(data))
	print "uint8_t __library_data[] = {"

	for i in range(0, len(data), 20):
		line = data[i:i+20]

		for val in line:
			v = struct.unpack(">B", val)

			print "0x%02x," % (v),
		print ""

	print "};"

if __name__ == "__main__":
	f = open(sys.argv[1])

	data = f.read()

	f.close()

	generate(data)
