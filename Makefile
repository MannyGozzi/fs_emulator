CC = gcc -g -Wall
CFLAGS = -std=c99 -pedantic -g
OBJS = fs_emulator

SOURCES = $(wildcard *.c)
EXECS = $(SOURCES:%.c=%)

all: $(EXECS)

clean :
	rm *.o $(MAIN) core