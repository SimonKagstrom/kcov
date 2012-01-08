KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

CFLAGS_test_module.o += -g3

obj-m += test_module.o

default:
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) modules

clean:
	rm -f *.o *.ko *.order *.symvers *.mod.c
