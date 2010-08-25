CFLAGS=`pkg-config --cflags glib-2.0`
LDFLAGS=`pkg-config --libs glib-2.0` -lelf -ldwarf -lpthread

%.o: %.c
	gcc -g -c -Iinclude $(CFLAGS) -Wall -o $@ $<

kcov: kc.o kc_line.o kc_addr.o kc_file.o kprobe_coverage.o main.o report.o utils.o kc_ptrace.o
	gcc -o $@ $+ $(LDFLAGS)

all: kcov


clean:
	rm -f kcov *.o
