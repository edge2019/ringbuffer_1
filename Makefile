ARCH ?=
CROSS_COMPILE ?=

CC := $(CROSS_COMPILE)gcc

all: main

main:main.c sbuf.c
	$(CC) $(CFLAGS) $^ -o $@

clean:
	-rm -f *.o main
