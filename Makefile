CFLAGS=-Wall -Werror -g

all: shell memory
memory: libmem.c memory.c
shell: libmem.c shell.c
