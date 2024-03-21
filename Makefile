OBJS	= libDisk.o libTinyFS.o tinyFSTest.o LL.o
SOURCE	= libDisk.c libTinyFS.c tinyFSTest.c LL.c
HEADER	= libDisk.h libTinyFS.h tinyFS_errno.h LL.h
OUT	= tinyFSTest
CC	 = gcc
FLAGS	 = -g -c -Wall -std=c99
LFLAGS	 = -lpthread -lm

all: $(OBJS)
	$(CC) -g $(OBJS) -o $(OUT) $(LFLAGS)

libDisk.o: libDisk.c
	$(CC) $(FLAGS) libDisk.c 

libTinyFS.o: libTinyFS.c
	$(CC) $(FLAGS) libTinyFS.c 

tinyFSTest.o: tinyFSTest.c
	$(CC) $(FLAGS) tinyFSTest.c

LL.o: LL.c
	$(CC) $(FLAGS) LL.c

clean:
	rm -f $(OBJS) $(OUT)