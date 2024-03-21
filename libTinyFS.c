
#define _GNU_SOURCE

#include "libTinyFS.h"

int disk_fd = -1;
LinkedList *LL = NULL;
int currentFD = 1;

/* Support function to clear blocks */

int freeBlocks(int disk, int numBlocks) {
   uint8_t superBlock[BLOCK_SIZE] = "";
   uint8_t freeBlock[BLOCK_SIZE] = "";
   Superblock *sbh = NULL;
   Block *bh = NULL;
   int blockCount = 0;
   int nextFB = -1;
   int currentFree = -1;
   int nextFBIdx = -1;
   
   if (disk < 0 || numBlocks < 0) {
      return TFS_ERROR_INVALID_PARAMETER;
   }

   // get superblock
   if (readBlock(disk, 0, superBlock) == -1) {
      return TFS_ERROR_DISK_READ;
   }

   sbh = (Superblock *)superBlock;

   // check number of free blocks (must be >= numBlocks)
   if (sbh->freeCount < numBlocks) {
      return TFS_ERROR_INVALID_PARAMETER;
   }

   // get the first available free block
   nextFBIdx = sbh->firstFreeBlock;

   // save starting free idx
   nextFB = nextFBIdx;

   do {
      // get the block
      if (readBlock(disk, nextFBIdx, freeBlock) == -1) {
         return TFS_ERROR_DISK_READ;
      }

      bh = (Block *)freeBlock;

      currentFree = nextFBIdx;
      nextFBIdx = bh->nextIdx;

      blockCount++;
   } while (blockCount < numBlocks);

   // update superblock with new next free block and counter
   sbh->freeCount -= numBlocks;
   sbh->firstFreeBlock = nextFBIdx;

   // update last free block to point to superblock
   bh->nextIdx = 0;

   // write the super block
   if (writeBlock(disk, 0, superBlock) == -1) {
      return TFS_ERROR_DISK_WRITE;
   }

   // write the last block in the chain
   if (writeBlock(disk, currentFree, freeBlock) == -1) {
      return TFS_ERROR_DISK_WRITE;
   }

   return nextFB;
}

int tfs_mkfs(char * filename, int nBytes) {
int numDisk = -1;
   int isFSValid = -1;

   // check disk
   if (diskExists(DISK_NAME) == 1) {
      if ((numDisk = openDisk(DISK_NAME, 0)) == -1) {
         return TFS_ERROR_DISK_OPEN;
      }
   }
   else {
      // create the disk and open it
      if ((numDisk = openDisk(DISK_NAME, DISK_SIZE)) == -1) {
         return TFS_ERROR_DISK_OPEN;
      }
   }

   // check FS exists
   if ((isFSValid = tfs_validate(numDisk)) == -1) {
      return TFS_ERROR_VALIDATE;
   }
   else if (isFSValid == 0) {
      tfs_format(numDisk);
   }

   return numDisk;
}

int tfs_mount(char * filename) {
   int numDisk = -1;
   int isFSValid = -1;

   // check disk
   if (diskExists(DISK_NAME) == 1) {
      if ((numDisk = openDisk(DISK_NAME, 0)) == -1) {
         return TFS_ERROR_DISK_OPEN;
      }
   }
   else {
      // create the disk and open it
      if ((numDisk = openDisk(DISK_NAME, DISK_SIZE)) == -1) {
         return TFS_ERROR_DISK_OPEN;
      }
   }

   // check FS exists
   if ((isFSValid = tfs_validate(numDisk)) == -1) {
      return TFS_ERROR_VALIDATE;
   }
   else if (isFSValid == 0) {
      tfs_format(numDisk);
   }

   return numDisk;
}

int tfs_openFile(char *name) {
   int inodeIdx = -1;
   FTE *fte = NULL;

   if (name == NULL) {
      return TFS_ERROR_INVALID_PARAMETER;
   }

   if (strnlen(name, MAX_FILENAME_LEN) == 0) {
      return TFS_ERROR_INVALID_PARAMETER;
   }

   
   if (disk_fd == -1) {
      if ((disk_fd = tfs_mount(DEFAULT_DISK_NAME)) == -1) {
         return TFS_ERROR_INVALID_FD;
      }

      // initialize the LL
      if (LL == NULL) {
         if (init(&LL, comparator) == -1) {
            return TFS_ERROR_BAD_ALLOC;
         }
      }
   }

   int i;
   for (i = 0; i < size(LL); i++) {
      fte = get(LL, i);
      if (fte == NULL) {
         return TFS_ERROR_BAD_ALLOC;
      }
      if (fte->fileName == NULL || name == NULL) {
         return TFS_ERROR_BAD_ALLOC;
      }
      if (strncmp(fte->fileName, name, MAX_FILENAME_LEN) == 0) {
         // file open
         return TFS_ERROR_BAD_ALLOC;
      }
   }

   // check for file by name
   if ((inodeIdx = findByName(name)) == -1) {
      return TFS_ERROR_BAD_ALLOC;
   }
   else if (inodeIdx == 0) { // file not found
      if ((inodeIdx = createFile(name)) == -1) {
         return TFS_ERROR_DISK_OPEN;
      }
   }

   // allocate file table entry
   if ((fte = (FTE *)calloc(1, sizeof(FTE))) == NULL) {
      return TFS_ERROR_BAD_ALLOC;
   }

   // initialize file table entry
   fte->fd = currentFD++;
   fte->inodeIdx = inodeIdx;
   fte->offset = 0;
   strncpy(fte->fileName, name, MAX_FILENAME_LEN);

   insert(LL, fte);

   return fte->fd;
}

int tfs_closeFile(fileDescriptor FD) {
   int fdIdx = -1;
   if (FD < 0) {
      return TFS_ERROR_INVALID_PARAMETER;
   }
   
   if (disk_fd == -1) {
      return TFS_ERROR_INVALID_PARAMETER;
   }

   // get fd
   if ((fdIdx = index(LL, FD)) == -1) {
      return TFS_ERROR_BAD_ALLOC;
   }

   // remove the fd and free data
   free(pop(LL, fdIdx));

   // LAST FILE
   if (size(LL) == 0) {
      destroy(&LL);
      LL = NULL;
      if (tfs_unmount() == -1) {
         return TFS_ERROR_DISK_MOUNTED;
      }
   }

   return 0;
}

int tfs_writeFile(fileDescriptor FD, char *buffer, int size) {
   uint8_t inodeBlock[BLOCK_SIZE] = "";
   uint8_t block[BLOCK_SIZE] = "";
   FTE *fte = NULL;
   Inode *inbh = NULL;
   Block *bh = NULL;
   int fdIdx = -1;
   int blockIdx = -1;
   uint32_t dataLength = -1;
   int blockTotal = -1;
   
   if (FD < 0 || buffer == NULL) {
      return TFS_ERROR_INVALID_PARAMETER;
   }
   
   if (disk_fd == -1) {
      return TFS_ERROR_INVALID_PARAMETER;
   }

   // truncate old data
   if (tfs_truncate(FD) == -1) {
      return TFS_ERROR_TRUNC;
   }

   // get the index of the fd in the "table" (aka linked list)
   if ((fdIdx = index(LL, FD)) == -1) {
      return TFS_ERROR_INVALID_FD;
   }

   // get the metadata - need the inode index
   fte = get(LL, fdIdx);

   // get inode block
   if (readBlock(disk_fd, fte->inodeIdx, inodeBlock) == -1) {
      return TFS_ERROR_DISK_READ;
   }

   inbh = (Inode *)inodeBlock;

   // # free blocks
   dataLength = strlen(buffer);
   blockTotal = (int)ceil((double)dataLength / MAX_DATA_SIZE);

   // get the free blocks
   if ((blockIdx = freeBlocks(disk_fd, blockTotal)) == -1) {
      return TFS_ERROR_BAD_ALLOC;
   }

   // update the inode
   inbh->block.nextIdx = blockIdx;
   inbh->fileSize = dataLength;
   inbh->block.type = inode;

   // write the updated inode block
   if (writeBlock(disk_fd, fte->inodeIdx, inodeBlock) == -1) {
      return TFS_ERROR_DISK_WRITE;
   }

   int i = 0;
   for (i = 0; i < blockTotal; i++) {
      // read the block
      if (readBlock(disk_fd, blockIdx, block) == -1) {
         return TFS_ERROR_DISK_READ;
      }
      bh = (Block *)block;

      // update the block
      bh->type = FEB;
      bh->magicNum = MAGIC_NUMBER;
      strncpy((char *)(block+sizeof(Block)), &buffer[i * MAX_DATA_SIZE],
               MAX_DATA_SIZE);

      // write the block
      if (writeBlock(disk_fd, blockIdx, block) == -1) {
         return TFS_ERROR_DISK_WRITE;
      }

      // update blockIdx and offset
      blockIdx = bh->nextIdx;
   }
   // reset fp
   fte->offset = 0;

   return 0;
}

int tfs_deleteFile(fileDescriptor FD) {
   //init
   int fdIdx = -1;

   uint8_t block[BLOCK_SIZE] = "";
   uint8_t superBlock[BLOCK_SIZE] = "";

   int nextBlockIdx = -1;
   int lastBlockIdx = -1;
   int freedCount = 0;

   FTE *fte = NULL;
   Block *bh = NULL;
   Superblock *sbh = NULL;

   if (FD < 0) {
      return TFS_ERROR_INVALID_PARAMETER;
   }
   
   if (disk_fd == -1) {
      return TFS_ERROR_INVALID_FD;
   }
   if ((fdIdx = index(LL, FD)) == -1) {
      return TFS_ERROR_BAD_ALLOC;
   }

   //get inode
   //printf("about to get inode\n.");
   if ((fte = get(LL, fdIdx)) == NULL) {
      return TFS_ERROR_BAD_ALLOC;
   }
   //printf("got inode\n");

   // free blocks
   nextBlockIdx = fte->inodeIdx;
   while(nextBlockIdx != 0) {
      // get the block
      if (readBlock(disk_fd, nextBlockIdx, block) == -1) {
         return TFS_ERROR_DISK_READ;
      }
      bh = (Block *)block;

      // update the next
      lastBlockIdx = nextBlockIdx;
      nextBlockIdx = bh->nextIdx;

      // update the block
      bh->type = FB;
      memset(block+sizeof(Block), 0, MAX_DATA_SIZE);

      freedCount++;

      // write the block
      if(writeBlock(disk_fd, lastBlockIdx, block) == -1) {
         return TFS_ERROR_DISK_WRITE;
      }
   }

   // get the superblock
   if (readBlock(disk_fd, 0, superBlock) == -1) {
      return TFS_ERROR_DISK_READ;
   }
   sbh = (Superblock *)superBlock;

   // update the next pointer in the last freed block
   bh->nextIdx = sbh->firstFreeBlock;

   // update superblock
   sbh->firstFreeBlock = fte->inodeIdx;
   sbh->freeCount += freedCount;

   // rewrite the last freed block
   if(writeBlock(disk_fd, lastBlockIdx, block) == -1) {
      return TFS_ERROR_DISK_WRITE;
   }

   // write the super block
   if (writeBlock(disk_fd, 0, superBlock) == -1) {
      return TFS_ERROR_DISK_WRITE;
   }

   // close file descriptor
   return tfs_closeFile(FD);
}

int tfs_readByte(fileDescriptor FD, char *buffer) {
   uint8_t block[BLOCK_SIZE] = "";
   FTE *fte = NULL;
   Block *bh = NULL;
   Inode *inbh = NULL;
   int fdIdx = -1;
   int blockTotal = 0;
   int blockIdx = -1;
   int targetBlocks = -1;

   if (FD < 0) {
      return TFS_ERROR_INVALID_PARAMETER;
   }

   if (disk_fd == -1) {
      return TFS_ERROR_INVALID_PARAMETER;
   }

   // get the index of the fd in the "table" (aka linked list)
   //printf("about to get index\n");
   if ((fdIdx = index(LL, FD)) == -1) {
      return TFS_ERROR_BAD_ALLOC;
   }
   //printf("index grabbed\n");

   // get inode index
   //printf("about to get inode idx\n");
   fte = get(LL, fdIdx);
   //print("got inode idx\n");

   // find # of blocks to move
   // TODO: do we need double here? may be causing errors
   targetBlocks = (int)ceil((double)(fte->offset + 1) / MAX_DATA_SIZE);

   // get the inode for the file
   if (readBlock(disk_fd, fte->inodeIdx, block) == -1) {
      return TFS_ERROR_DISK_READ;
   }

   inbh = (Inode *)block;

   // ensure not reading past end of file
   if ((fte->offset) >= (inbh->fileSize)) {
      buffer[0] = '\0';
      return TFS_ERROR_BAD_ALLOC;
   }

   // get the first data block
   blockIdx = inbh->block.nextIdx;

   // move to the correct block
   do {

      // get the current block
      if (readBlock(disk_fd, blockIdx, block) == -1) {
         return TFS_ERROR_DISK_READ;
      }

      bh = (Block *)block;

      // update block index
      blockIdx = bh->nextIdx;

      blockTotal++;
   } while (blockTotal < targetBlocks);

   // write bit
   //printf("about to write bit\n");
   buffer[0] = block[sizeof(Block) +
                      (fte->offset % MAX_DATA_SIZE)];
   //printf("wrote bit!!\n");
   // update fp
   if (buffer[0] != '\0') {
      (fte->offset)++;
   }

   return 0;
}

int tfs_seek(fileDescriptor FD, int offset) {
   int fdIdx = -1;
   FTE *fte = NULL;

   if (FD < 0 || offset < 0) {
      return TFS_ERROR_INVALID_PARAMETER;
   }

   if (disk_fd == -1) {
      return TFS_ERROR_INVALID_FD;
   }

   // get fd
   if ((fdIdx = index(LL, FD)) == -1) {
      return TFS_ERROR_BAD_ALLOC;
   }

   // get inode idx
   fte = get(LL, fdIdx);

   // update fte
   fte->offset = offset;

   return 0;
}

int tfs_rename(fileDescriptor FD, char *filename) {
   uint8_t block[BLOCK_SIZE] = "";
   FTE *fte = NULL;
   Inode *inbh = NULL;
   int fdIdx = -1;

   if (FD < 0 || filename == NULL) {
      return TFS_ERROR_INVALID_PARAMETER;
   }

   if (disk_fd == -1) {
      return TFS_ERROR_INVALID_FD;
   }

   // get fd index
   if ((fdIdx = index(LL, FD)) == -1) {
      return TFS_ERROR_BAD_ALLOC;
   }

   fte = get(LL, fdIdx);

   // retrieve inode
   if (readBlock(disk_fd, fte->inodeIdx, block) == -1) {
      return TFS_ERROR_DISK_READ;
   }
   inbh = (Inode *)block;

   // rename file
   strncpy(inbh->filename, filename, MAX_FILENAME_LEN);

   // write updated inode to disk
   if (writeBlock(disk_fd, fte->inodeIdx, block) == -1) {
      return TFS_ERROR_DISK_WRITE;
   }

   return 0;
}

time_t tfs_getTimeStamp(fileDescriptor FD) {
   uint8_t block[BLOCK_SIZE] = "";
   FTE *fte = NULL;
   Inode *inbh = NULL;
   int fdIdx = -1;

   if (FD < 0) {
      return TFS_ERROR_INVALID_PARAMETER;
   }

   if (disk_fd == -1) {
      return TFS_ERROR_INVALID_FD;
   }

   // get the fd from LL
   if ((fdIdx = index(LL, FD)) == -1) {
      return TFS_ERROR_BAD_ALLOC;
   }

   // get the inode from FTE
   fte = get(LL, fdIdx);

   // get the inode for the file
   if (readBlock(disk_fd, fte->inodeIdx, block) == -1) {
      return TFS_ERROR_DISK_READ;
   }
   inbh = (Inode *)block;

   return inbh->creationTime;
}

int tfs_truncate(int FD) {
   uint8_t block[BLOCK_SIZE] = "";
   uint8_t superBlock[BLOCK_SIZE] = "";
   int fdIdx = -1;
   FTE *fte = NULL;
   Block *bh = NULL;
   Superblock *sbh = NULL;
   Inode *inbh = NULL;
   int nextBlockIdx = -1;
   int lastBlockIdx = -1;
   int freedCount = 0;
   int firstFreedBlock = -1;

   
   if (disk_fd == -1) {
      return TFS_ERROR_INVALID_FD;
   }

   // check the is open
   // get the fd from LL
   if ((fdIdx = index(LL, FD)) == -1) {
      return TFS_ERROR_BAD_ALLOC;
   }

   // get inode index
   fte = get(LL, fdIdx);

   // get the inode block
   if (readBlock(disk_fd, fte->inodeIdx, block) == -1) {
      return TFS_ERROR_DISK_READ;
   }
   inbh = (Inode *)block;

   // get the first data block
   nextBlockIdx = inbh->block.nextIdx;

   if (nextBlockIdx == 0) {
      // no data, we are done
      return 0;
   }
   else {
      firstFreedBlock = nextBlockIdx;
   }

   // update the inode
   inbh->fileSize = 0;
   inbh->block.nextIdx = 0;

   // write the updated inode block
   if(writeBlock(disk_fd, fte->inodeIdx, block) == -1) {
      return TFS_ERROR_DISK_WRITE;
   }

   // loop over the data blocks and free them from their prison
   while (nextBlockIdx != 0) {
      // get the data block
      if (readBlock(disk_fd, nextBlockIdx, block) == -1) {
         return TFS_ERROR_DISK_READ;
      }
      bh = (Block *)block;

      // update the next block to free
      lastBlockIdx = nextBlockIdx;
      nextBlockIdx = bh->nextIdx;

      // update the block
      bh->type = FB;
      memset(block+sizeof(Block), 0, MAX_DATA_SIZE);

      freedCount++;

      // write the block
      if(writeBlock(disk_fd, lastBlockIdx, block) == -1) {
         return TFS_ERROR_DISK_WRITE;
      }
   }

   // get the superblock
   if (readBlock(disk_fd, 0, superBlock) == -1) {
      return TFS_ERROR_DISK_READ;
   }
   sbh = (Superblock *)superBlock;

   // update the next pointer in the last freed block
   bh->nextIdx = sbh->firstFreeBlock;

   // update superblock
   sbh->firstFreeBlock = firstFreedBlock;
   sbh->freeCount += freedCount;

   // rewrite the last freed block
   if(writeBlock(disk_fd, lastBlockIdx, block) == -1) {
      return TFS_ERROR_DISK_WRITE;
   }

   // write the super block
   if (writeBlock(disk_fd, 0, superBlock) == -1) {
      return TFS_ERROR_DISK_WRITE;
   }

   return 0;
}

int tfs_validate(fileDescriptor disk) {
   uint8_t block[BLOCK_SIZE] = "";
   Superblock *sbh = NULL;

   // get super block (block 0)
   if (readBlock(disk, 0, block) == -1) {
      return TFS_ERROR_DISK_READ;
   }

   // cast to superblock and validate magic num
   sbh = (Superblock *)block;

   if (sbh->block.type == sb &&
       sbh->block.magicNum == MAGIC_NUMBER) {
      return 1;
   }

   return 0;
}

/* Support function for clearing before exit */

int tfs_format(int disk) {
   uint8_t block[BLOCK_SIZE] = "";
   Superblock *sbh = (Superblock *)block;
   Block *bh = (Block *)block;
   int diskSize = -1;

   // create Superblock
   sbh->block.type = sb;
   sbh->block.magicNum = MAGIC_NUMBER;
   if ((diskSize = getDiskSize(disk)) == -1) {
      return TFS_ERROR_DISK_FULL;
   }
   sbh->freeCount = ((diskSize / BLOCK_SIZE) - 1);
   sbh->firstFreeBlock = 1;

   if (writeBlock(disk, 0, block) == -1) {
      return TFS_ERROR_DISK_WRITE;
   }

   // clean up
   sbh->freeCount = 0;
   sbh->firstFreeBlock = 0;

   // loop through the entire disk
   int i = 0;
   for (i = 1; ((diskSize / BLOCK_SIZE) - 1); i++) {
      bh->type = FB;
      bh->magicNum = MAGIC_NUMBER;
      bh->nextIdx = (i + 1);
      memset(block+sizeof(Block), 0, MAX_DATA_SIZE);

      // write free block
      if (writeBlock(disk, i, block) == -1) {
         return TFS_ERROR_DISK_WRITE;
      }
   }

   // point end to superblock (0)
   bh->nextIdx = 0;
   if (writeBlock(disk, (i-1), block) == -1) {
      return TFS_ERROR_DISK_WRITE;
   }

   return 0;
}

int findByName(char *filename) {
   uint8_t block[BLOCK_SIZE] = "";
   Block *bh = NULL;
   Inode *ibh = NULL;

   if (filename == NULL) {
      return TFS_ERROR_INVALID_PARAMETER;
   }

   if (tfs_validate(disk_fd) != 1) {
      return TFS_ERROR_VALIDATE;
   }

   // loop through the entire disk, checking all inodes for our file name
   int i = 0;
   for (i = 1; readBlock(disk_fd, i, block) == 0; i++) {
      // check the type of the block
      bh = (Block *)block;
      if (bh->type != inode) {
         continue;
      }

      // check inode name
      ibh = (Inode *)block;
      if (ibh == NULL || ibh->filename == NULL) {
         continue;
      }
      if (strncmp(filename, ibh->filename, MAX_FILENAME_LEN) == 0) {
         // match
         return i;
      }
   }

   //file not found, dont throw error
   return 0;
}


int tfs_unmount() {
   if (closeDisk(disk_fd) == -1) {
      return TFS_ERROR_DISK_OPEN;
   }

   // set disk to closed
   disk_fd = -1;

   return 0;
}

int createFile(char *filename) {
   uint8_t block[BLOCK_SIZE] = "";
   Inode *inbh = NULL;
   Superblock *sbh = NULL;
   int blockIdx = -1;

   
   if (filename == NULL) {
      return TFS_ERROR_INVALID_PARAMETER;
   }

   // check disk
   if (disk_fd == -1) {
      return TFS_ERROR_INVALID_FD;
   }

   // get superblock
   if (readBlock(disk_fd, 0, block) == -1) {
      return TFS_ERROR_DISK_READ;
   }

   sbh = (Superblock *)block;

   // check number of free blocks 
   // needs to be 2 for inode + starting block
   if (sbh->freeCount < 2) {
      return TFS_ERROR_INVALID_PARAMETER;
   }

   // clean up block
   memset(block, 0, BLOCK_SIZE); 

   // new free block
   if ((blockIdx = freeBlocks(disk_fd, 1)) == -1) {
      return TFS_ERROR_BAD_ALLOC;
   }

   inbh = (Inode *)block;

   // update inode
   inbh->block.type = inode;
   inbh->block.magicNum = MAGIC_NUMBER;
   inbh->block.nextIdx = 0;
   if (strncpy(inbh->filename, filename, MAX_FILENAME_LEN) == NULL) {
      return TFS_ERROR_BAD_ALLOC;
   }
   inbh->creationTime = time(NULL);
   inbh->fileSize = 0;

   // write the updated inode to disk
   if (writeBlock(disk_fd, blockIdx, block) == -1) {
      return TFS_ERROR_DISK_WRITE;
   }

   return blockIdx;
}
/*

DEPRECATED TESTING FUNCTIONS - DO NOT USE

void test_tfs_mount() {
    printf("Testing tfs_mount...\n");
    int ret = tfs_mount("test_disk.bin");
    if (ret == TFS_SUCCESS) {
        printf("tfs_mount test passed!\n");
    } else {
        printf("tfs_mount test failed with error code: %d\n", ret);
    }
}

void test_tfs_unmount() {
    printf("Testing tfs_unmount...\n");
    int ret = tfs_unmount();
    if (ret == TFS_SUCCESS) {
        printf("tfs_unmount test passed!\n");
    } else {
        printf("tfs_unmount test failed with error code: %d\n", ret);
    }
}

void test_tfs_openFile() {
    printf("Testing tfs_openFile...\n");
    int fd = tfs_openFile("test.txt");
    if (fd >= 0) {
        printf("tfs_openFile test passed! File descriptor: %d\n", fd);
    } else {
        printf("tfs_openFile test failed with error code: %d\n", fd);
    }
}

void test_tfs_writeFile() {
    printf("Testing tfs_writeFile...\n");
    int fd = tfs_openFile("test.txt");
    if (fd >= 0) {
        char buffer[] = "Hello, TinyFS!";
        int ret = tfs_writeFile(fd, buffer, strlen(buffer));
        if (ret == TFS_SUCCESS) {
            printf("tfs_writeFile test passed!\n");
        } else {
            printf("tfs_writeFile test failed with error code: %d\n", ret);
        }
    } else {
        printf("Failed to open file for writing.\n");
    }
}

void test_tfs_deleteFile() {
    printf("Testing tfs_deleteFile...\n");
    int fd = tfs_openFile("test.txt");
    if (fd >= 0) {
        int ret = tfs_deleteFile(fd);
        if (ret == TFS_SUCCESS) {
            printf("tfs_deleteFile test passed!\n");
        } else {
            printf("tfs_deleteFile test failed with error code: %d\n", ret);
        }
    } else {
        printf("Failed to open file for deletion.\n");
    }
}

int main() {
    test_tfs_mkfs();
    test_tfs_mount();
    test_tfs_openFile();
    test_tfs_writeFile();
    test_tfs_deleteFile();
    test_tfs_unmount();

    return 0;
}
*/

