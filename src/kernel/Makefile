KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

CFLAGS_kprobe-coverage.o += -g3

obj-m += kprobe-coverage.o

default:
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) modules

clean:
	rm -f *.o *.ko *.order *.symvers *.mod.c
