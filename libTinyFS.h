
#ifndef __LIB_TINY_FS_H_
#define __LIB_TINY_FS_H_

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include "libDisk.h"
#include "LL.h"
#include "tinyFS_errno.h"

#define DEFAULT_DISK_NAME "tinyFSDisk"
#define DEFAULT_DISK_SIZE 10240
#define BLOCK_SIZE 256
#define MAX_FILENAME_LEN 8
#define MAGIC_NUMBER 0x44
#define MAX_BLOCKS 10240 //unused
#define MAX_FILES 10
#define BLOCK_MAGIC_NUM 45
#define MAX_DATA_SIZE (BLOCK_SIZE - sizeof(Block))

typedef int fileDescriptor;

typedef enum {
   sb = 1,
   inode = 2,
   FEB = 3,
   FB = 4,
} Blocks;

struct block {
   uint8_t type;
   uint8_t magicNum;
   uint8_t nextIdx;
   uint8_t empty;
};

typedef struct block Block;

struct sb {
   //hold freecount and first free block
   struct block block;
   uint32_t freeCount;
   uint32_t firstFreeBlock;
};

typedef struct sb Superblock;

struct inodeBlock {
   struct block block;
   char filename[MAX_FILENAME_LEN];
   uint32_t fileSize; //uint32_t SHOULD work
   time_t creationTime; //untested
};

typedef struct inodeBlock Inode;

typedef struct {
   int fd;
   int inodeIdx; 
   int offset;   // **offset from start of file
   char fileName[MAX_FILENAME_LEN];
} FTE;

int tfs_mkfs(char * filename, int nBytes);

int tfs_mount(char * filename);

int tfs_unmount(void);

int tfs_validate(fileDescriptor disk);

int tfs_openFile(char *name);

int tfs_closeFile(fileDescriptor FD);

int tfs_writeFile(fileDescriptor FD, char *buffer, int size);

int tfs_deleteFile(fileDescriptor FD);

int tfs_readByte(fileDescriptor FD, char *buffer);

int tfs_seek(fileDescriptor FD, int offset);

int tfs_rename(fileDescriptor FD, char *filename); //untested

time_t tfs_getTimeStamp(fileDescriptor FD);

#endif // __LIB_TINY_FS_H_
