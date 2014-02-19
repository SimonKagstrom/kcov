#!/usr/bin/env python

import sys, struct

def generate(data_in, base_name):
	print "#include <stdint.h>"
	print "#include <stdlib.h>"

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
	if len(sys.argv) < 3:
		sys.exit(1)

	f = open(sys.argv[1])
	data = f.read()
	f.close()

	generate(data, sys.argv[2])
