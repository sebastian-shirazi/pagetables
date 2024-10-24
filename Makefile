CC = clang
CFLAGS = -Wall -g -Wextra
LDFLAGS =

all: libmlpt.a

libmlpt.a: pagetable.o
	ar rcs $@ $^

pagetable.o: pagetable.c mlpt.h config.h
	$(CC) $(CFLAGS) -c pagetable.c

clean:
	rm -f *.o libmlpt.a

.PHONY: all clean