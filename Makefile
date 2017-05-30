CC=gcc
CPPFLAGS=-MMD
CFLAGS=-Wall -Wextra -Werror -std=c99
LDLIBS=-lcrypt -lgmp

BINARIES = oussh password ousshd keygen

all: $(BINARIES)

oussh: oussh.o pts.o password.o  rsa.o
ousshd: ousshd.o pts.o password.o rsa.o
password: password.o password_demo.o
keygen: rsa.o keygen.o
tea: tea.c

.PHONY: clean

clean:
	rm -f *.o
	rm -f *.d
	rm -f $(BINARIES)

-include *.d
