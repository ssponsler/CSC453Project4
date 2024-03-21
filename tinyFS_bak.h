#include "disk.h"
#include <stdint.h>
#include <math.h>
#include "LL.h"
#include "tinyFS_errno.h"

#define BLOCKSIZE 256
#define DEFAULT_DISK_SIZE 10240
#define DEFAULT_DISK_NAME "tinyFSDisk"
#define MAGIC_NUMBER 0x44
#define MAX_FILENAME_LENGTH 16
#define INODE_BLOCK_SIZE 32
#define MAX_FILES 10
#define MAX_BLOCKS 102400

#define min(a, b) ((a) < (b) ? (a) : (b))

typedef int fileDescriptor;

typedef union Block Block; // forward declaration
typedef struct Inode Inode; // forward declaration

struct Inode {
    // 2
    uint8_t code; // 1 byte
    char name[MAX_FILENAME_LENGTH]; // 16 bytes
    int idx; // sizeof(int)
    // size in blocks
    int size; // 1 byte
    uint8_t magicNumber; // 1 byte
    // * dataBlockHead; // 8 bytes
    LinkedList * dataBlocks; // 8 bytes
    Inode * nextInode; // 8 bytes
    char padding[BLOCKSIZE - (26 + MAX_FILENAME_LENGTH + (2*sizeof(int)))]; // padding
}; // total: 256 bytes

typedef struct {
    // 1
    uint8_t code; // 1 byte
    uint8_t magicNumber; // 1 byte
    LinkedList * inodesList; // 8 bytes
    //LinkedList * blockList; // 8 bytes
    char padding[BLOCKSIZE-18]; //padding
} Superblock; // total: 256 bytes
/*
A file extent block is a fixed sized block that contains file data and (optionally) a pointer to the next
data block. If the file extent is the last (or only) block, the remaining bytes and the pointer should
be set to 0x00
*/
typedef struct {
    uint8_t code;                 // 1 byte
    uint8_t magicNumber;          // 1 byte
    int bNum;                     // 4 bytes
    char padding[4];              // 4 bytes (explicit padding for alignment)
    Block *nextBlockPtr;          // 8 bytes
    Block *inodePtr;              // 8 bytes
    char data[BLOCKSIZE - 32];    // 230 bytes (256 - 26)
} FileExtentBlock;

typedef struct {
    // 4
    uint8_t code; // 1 byte
    uint8_t magicNumber; // 1 byte
    //int blockNum; // sizeof(int) bytes
    Block * nextFB; // 8 bytes
    char data[BLOCKSIZE-18]; // 256 - 10 bytes
} FreeBlock;

union Block {
    Inode inode;
    Superblock superblock;
    FileExtentBlock fileExtentBlock;
    FreeBlock freeBlock;
    int code;
    //char data[256];
};

Superblock * superblock;
//Inode * inodes[MAX_FILES]; // holds inodes for each file
LinkedList * inodes; //holds inodes for each file
LinkedList * freeBlocksList;
//LinkedList * list; // holds all data blocks

FileExtentBlock * convertToFEB(FreeBlock * fb, Inode * inode);
int freeBlock(Block * block);
int tfs_mkfs(char* filename, int nBytes);
int tfs_mount(char* diskname);
int tfs_unmount(void);
int tfs_openFile(char* name);
int tfs_closeFile(int FD);
int tfs_writeFile(int FD, char* buffer, int size);
int tfs_deleteFile(int FD);
int tfs_readByte(int FD, char* buffer);
int tfs_seek(int FD, int offset);
