CC=gcc
CFLAGS= -Wall -Wextra -Wpedantic -Werror -Wshadow -Wformat=2 -fno-common -g3 -pthread

all: ph_server fs

ph_server: ph.o msg.o
fs: msg.o ph_client.o

ph_client.o: msg.o
ph.o:
msg.o:

.PHONY:
clean:
	rm -f *.o ph_server
