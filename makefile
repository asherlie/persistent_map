CC=gcc
CFLAGS= -Wall -Wextra -Wpedantic -Werror -Wshadow -Wformat=2 -fno-common -g3 -pthread -lpcap 

all: ph

ph:

.PHONY:
clean:
	rm -f ph
