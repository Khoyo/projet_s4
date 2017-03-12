CC=gcc
CFLAGS=-Wall -Wextra -Werror -std=c99
LDFLAGS=-lcrypt

%.o: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -MMD -c -o $@ $<


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
