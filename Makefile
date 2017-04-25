CC=gcc
CPPFLAGS=-MMD
CFLAGS=-Wall -Wextra -Werror -std=c99
LDFLAGS=-lcrypt -lgmp


all: oussh password ousshd

oussh: oussh.o pts.o password.o
ousshd: ousshd.o pts.o password.o
password: password.o password_demo.o
keygen: rsa.c
tea: tea.c

.PHONY: clean

clean:
	rm -f *.o
	rm -f *.d
	rm oussh password ousshd

-include *.d
