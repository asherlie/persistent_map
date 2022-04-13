CC=gcc
CFLAGS= -Wall -Wextra -Wpedantic -Werror -Wshadow -Wformat=2 -fno-common -g3 -pthread -lpcap 

all: ph_server

ph_server: ph.o

ph.o:

.PHONY:
clean:
	rm -f *.o ph_server
