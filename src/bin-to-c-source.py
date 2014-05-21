#!/usr/bin/env python

import sys, struct

def generate(data_in, base_name):
	print "size_t %s_data_size = 0x%x;" % (base_name, len(data))
	print "uint8_t %s_data[] = {" % (base_name)

	for i in range(0, len(data), 20):
		line = data[i:i+20]

		for val in line:
			v = struct.unpack(">B", val)

			print "0x%02x," % (v),
		print ""

	print "};"

if __name__ == "__main__":
	if len(sys.argv) < 3 or (len(sys.argv) - 1) % 2 != 0:
		print "Usage: bin-to-source.py <file> <base-name> [<file2> <base-name2>]"
		sys.exit(1)

	print "#include <stdint.h>"
	print "#include <stdlib.h>"

	for i in range(1,len(sys.argv), 2):
		file = sys.argv[i]
		base_name = sys.argv[i + 1]

		f = open(file)
		data = f.read()
		f.close()

		generate(data, base_name)
