CC=gcc
CPPFLAGS=-MMD
CFLAGS=-Wall -Wextra -Werror -std=c99
LDFLAGS=-lcrypt


all: oussh password ousshd

oussh: oussh.o pts.o
ousshd: ousshd.o pts.o
password: password.o

.PHONY: clean

clean:
	rm -f *.o
	rm -f *.d
	rm oussh password ousshd

-include *.d
