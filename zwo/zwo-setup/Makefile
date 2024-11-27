CC=g++ -O2 -Wall
# CFLAGS=-I../include/
# LD=
# LDIR=
# LIBS=

binaries=zwo-setup

all: clean $(binaries)

zwo-setup: zwo-setup.cpp
	$(CC) $? -o zwo-setup.out

run:
	./zwo-setup.out

.PHONY: clean
clean:
	rm -f $(binaries) *.o *.out
