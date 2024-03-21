#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

#define BLOCKSIZE 256
#define DEFAULT_DISK_NAME "tinyFSDisk"
#define MAX_DATA_BLOCKS 102400

typedef struct Disk {
    int fd;
    int size;
    int nBlocks;
    time_t timeStamp;
    char *name;
    struct Disk* next;
} Disk;

int writeBlock(int fd, int bNum, void* block);
int readBlock(int fd, int bNum, void* block);
int closeDisk(int fd);
int openDisk(char* filename, int nBytes);
int isDiskExtant(char *filename);
int getDiskSize(int disk);
void releaseBlock(int blockNum);
