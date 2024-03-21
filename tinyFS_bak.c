#include "tinyFS.h"

int mountedDisk = -1; 
// this gave me VERY weird make bugs with tinyFSTest.c even when it was
// not included anywhere in that file and all circular dep's were cleared.

int current_fd = -1; // current disk
int disk_fd = -1;
int openFiles = 0;
int freeBlocks = 0;
int filePointer = 0;
int lastBlockIndex = 0;
int firstBlockLocation = 0;

static Inode * activeInode = NULL;

static char *mounted_disk = NULL;

/* Support function to convert a FreeBlock to FileExtentBlock */
FileExtentBlock * convertToFEB(FreeBlock * fb, Inode * inode) {
   freeBlocks--;
   FileExtentBlock * FEB = NULL;
   FEB = (FileExtentBlock *)malloc(sizeof(FileExtentBlock));
   FEB->code = 3;
   FEB->bNum = -1;
   FEB->magicNumber = MAGIC_NUMBER;
   FEB->nextBlockPtr = NULL;
   (FEB->inodePtr) = inode;
   free(fb);
   return FEB;
}

/* Support function to clear block data */
int freeBlock(Block * block) {
   if (!block) {
      return TFS_ERROR_INVALID_PARAMETER;
   }
   if (openFiles == 0) {
      //this is the last file
      //tfs_closeFile(block->idx);

      //return TFS_SUCCESS;
   }
   if (block == NULL) {
       return TFS_ERROR_INVALID_PARAMETER;
   }
   if ((block->code) == 2) {

      /*
      TODO: convert all blocks in inode to FreeBlocks
      */
      Inode * inode = NULL;
      inode = &(block->inode);
      if (!inode) {
         return TFS_ERROR_BAD_ALLOC;
      }
      //Inode
      //We are closing the file
      int blockSize = sizeOfLL(inode->dataBlocks);
      int i = 0;
      for (i = 0; i < blockSize; i++) {
         // Clear associated node data
         // increments freeBlocks
         freeBlock(getLL(inode->dataBlocks, i));
      }
      openFiles--;
      inode->size = 0;
      inode->magicNumber = MAGIC_NUMBER;
      strncpy(inode->name, DEFAULT_DISK_NAME, MAX_FILENAME_LENGTH-1);
      inode->name[MAX_FILENAME_LENGTH-1]= '\0';

      //write updated inode back to disk
      if (writeBlock(mountedDisk, inode->idx, inode) < 0) {
          printf("freeBlock: Could not write updated inode to disk.\n");
          return TFS_ERROR_DISK_WRITE;
      }
      return TFS_SUCCESS;
   }
   if ((block->code) == 3) {
      //FileExtentBlock
      //convert to FreeBlock
      /*
      TODO: release block, if last block, decrement lastBlockIndex
      write empty data into block
      */
      FileExtentBlock * feb = NULL;
      feb = &(block->fileExtentBlock);
      if (feb == NULL) {
         printf("freeBlock: Could not retrieve FEB from block.\n");
         return TFS_ERROR_BAD_ALLOC;
      }
      FreeBlock * fb = NULL;
      fb = (FreeBlock*)malloc(sizeof(FreeBlock));
      fb->magicNumber = MAGIC_NUMBER;
      //fb->blockNum = -1;
      fb->nextFB = NULL;
      fb->code = 4;
      insertIntoLL(freeBlocksList, fb);
      freeBlocks++;
      int bNum = feb->bNum;
      if ((writeBlock(mountedDisk, bNum, fb)) < 0) {
          printf("freeBlock: Could not write empty data to disk.\n");
          return TFS_ERROR_DISK_WRITE;
      }
      if ((bNum) == lastBlockIndex) {
         if (lastBlockIndex > 0) {
            lastBlockIndex--;
         }
      }
      free(block);
      return TFS_SUCCESS;
   }
   else {
      // not for use w/ superblock or free block
      return TFS_ERROR_INVALID_PARAMETER;
   }
}

/* 
   Makes a blank TinyFS file system of size nBytes on the unix file
   specified by ‘filename’. This function should use the emulated disk
   library to open the specified unix file, and upon success, format the
   file to be a mountable disk. This includes initializing all data to 0x00,
   setting magic numbers, initializing and writing the superblock and
   inodes, etc. Must return a specified success/error code. 
   TODO: set to default filename if none given
 */

int tfs_mkfs(char* filename, int nBytes) {
   printf("entering tfs_mkfs()\n");
   //sanity tests
   if (sizeof(FileExtentBlock) != BLOCKSIZE) {
      printf("FEB Block size mismatch: %ld\n", sizeof(FileExtentBlock));
      return TFS_ERROR_UNK;
   }
   if (sizeof(FreeBlock) != BLOCKSIZE) {
      printf("FB Block size mismatch: %ld\n", sizeof(FreeBlock));
      return TFS_ERROR_UNK;
   }
   if (sizeof(Superblock) != BLOCKSIZE) {
      printf("Superblock Block size mismatch: %ld\n", sizeof(Superblock));
      return TFS_ERROR_UNK;
   }
   if (sizeof(Inode) != BLOCKSIZE) {
      printf("Inode Block size mismatch: %ld\n", sizeof(Inode));
      return TFS_ERROR_UNK;
   }


    if (nBytes < BLOCKSIZE) {
        printf("tfs_mkfs: Disk size must be at least %d bytes.\n", BLOCKSIZE);
        return -1;
    }

    mountedDisk = openDisk(filename, nBytes);
    //mountedDisk = fd;
    if (mountedDisk < 0) {
        printf("tfs_mkfs: Could not open disk.\n");
        return TFS_ERROR_DISK_OPEN;
    }

    inodes = (LinkedList *)malloc(sizeof(LinkedList));
    freeBlocksList = (LinkedList *)malloc(sizeof(LinkedList));

    if (initLL(&(inodes)) == -1) {
       printf("Could not initialize LL.\n");
       //closeDisk(mountedDisk);
       return TFS_ERROR_BAD_ALLOC;
    }
    if (initLL(&(freeBlocksList)) == -1) {
       printf("Could not initialize LL.\n");
       //closeDisk(mountedDisk);
       return TFS_ERROR_BAD_ALLOC;
    }

    int totalBlocks = nBytes / BLOCKSIZE;
    // round up?
    //if (nBytes % BLOCKSIZE > 0) {
    //   totalBlocks++;
    //}

    // Initialize superblock
    superblock = (Superblock *)calloc(1, sizeof(Superblock));
    if (superblock == NULL) {
       //closeDisk(mountedDisk);
       return TFS_ERROR_BAD_ALLOC;
    }

    superblock->code = 1;
    superblock->magicNumber = MAGIC_NUMBER;

    int i = 0;

    // Initialize MAX_FILES inodes

    // calloc to set padding bits to 0x00
    Inode * head = (Inode *)calloc(1, sizeof(Inode));
    if (!head) {
       //closeDisk(mountedDisk);
       return TFS_ERROR_BAD_ALLOC;
    }
    //LinkedList * headInodeDataList = NULL;
    if (initLL(&(head->dataBlocks)) == -1) {
       //closeDisk(mountedDisk);
       return TFS_ERROR_BAD_ALLOC;
    }
    head->code = 2;
    strncpy(head->name, filename, MAX_FILENAME_LENGTH-1);
    head->name[MAX_FILENAME_LENGTH-1]= '\0';
    head->magicNumber = MAGIC_NUMBER;
    head->size = 0;
    head->idx = 0;
    //head->dataBlocks = &headInodeDataList;
    insertIntoLL(inodes, head);
    superblock->inodesList = inodes;

    // We can now write the superblock to disk
    if (writeBlock(mountedDisk, 0, &superblock)) {
       printf("init: could not write superblock.\n");
       //closeDisk(mountedDisk);
       return TFS_ERROR_DISK_WRITE;
    }
    lastBlockIndex++;
    
    Inode * temp = head;
    for (i = 1; i < MAX_FILES; i++) {
       Inode * newInode = (Inode *)calloc(1, sizeof(Inode));
       if (!newInode) {
          //closeDisk(mountedDisk);
          return TFS_ERROR_BAD_ALLOC;
       }
       //LinkedList * list = NULL; // holds data blocks for file
       if (initLL(&(newInode->dataBlocks)) == -1) {
          printf("Could not initialize LL.\n");
          //closeDisk(mountedDisk);
          return TFS_ERROR_BAD_ALLOC;
       }
       newInode->code = 2;
       newInode->size = 0;
       newInode->idx = i;
       //newInode->dataBlocks = &list;
       strncpy(newInode->name, filename, MAX_FILENAME_LENGTH-1);
       newInode->name[MAX_FILENAME_LENGTH-1]= '\0';
       temp->nextInode = newInode;
       insertIntoLL(inodes, newInode);
       // Write this inode to disk
       if (writeBlock(mountedDisk, i, &temp)) {
          printf("init: could not write superblock.\n");
          //closeDisk(mountedDisk);
          return TFS_ERROR_DISK_WRITE;
       }
       lastBlockIndex++;
       temp = newInode;
    }

    //firstBlockLocation: first readable block with data for readByte()
    // 256 (superblock) + n*256(n=MAX_FILES)
    firstBlockLocation = BLOCKSIZE + (MAX_FILES * BLOCKSIZE);

    free(temp);

    freeBlocks = totalBlocks - 1 - (MAX_FILES);

    // initialize remaining blocks as free blocks
    // to be converted to FEBlocks later
    FreeBlock * fb = (FreeBlock *)calloc(freeBlocks, sizeof(FreeBlock));
    if (!fb) {
       //closeDisk(mountedDisk);
       return TFS_ERROR_BAD_ALLOC;
    }
    fb[0].magicNumber = MAGIC_NUMBER;
    fb[0].code = 4;
    insertIntoLL(freeBlocksList, &fb[0]);

    for (i = 1; i < freeBlocks ; i++) {
      fb[i].magicNumber = MAGIC_NUMBER;
      fb[i].code = 4;
      fb[i-1].nextFB = &fb[i];
      insertIntoLL(freeBlocksList, &fb[i]);
    }

    fb[freeBlocks-1].nextFB = NULL;

    //closeDisk(mountedDisk);
    return TFS_SUCCESS;
}



int tfs_mount(char* diskname) {
    if (mountedDisk >= 0) {
        printf("tfs_mount: A disk is already mounted.\n");
        //return TFS_ERROR_DISK_MOUNTED;
        return TFS_SUCCESS;
    }

    mountedDisk = openDisk(diskname, 0);
    if (mountedDisk < 0) {
        printf("tfs_mount: Could not open disk.\n");
        return TFS_ERROR_DISK_OPEN;
    }
    if (superblock == NULL) {
       //first mount, mkfs has not been called
       return TFS_ERROR_INVALID_FS;
    }
    // Read superblock
    printf("attempting to read block on mountedDisk: %d\n", mountedDisk);
    printf("current file_fd: %d\n", mountedDisk);
    if (readBlock(mountedDisk, 0, &superblock) < 0) {
        printf("tfs_mount: Could not read superblock.\n");
        //closeDisk(mountedDisk);
        mountedDisk = -1;
        return TFS_ERROR_DISK_OPEN;
    }

    if (superblock->magicNumber != MAGIC_NUMBER) {
       printf("tfs_mount: Invalid magic number: %d, expected %d.\n", superblock->magicNumber, MAGIC_NUMBER);
       //closeDisk(mountedDisk);
       mountedDisk = -1;
       return TFS_ERROR_BAD_MAGIC;
    }


    // Read first inode
    Inode * temp = NULL;
    printf("attempting to read block on mountedDisk: %d\n", mountedDisk);
    printf("current file_fd: %d\n", mountedDisk);
    if (readBlock(mountedDisk, 1, &temp) < 0) {
        printf("tfs_mount: Could not read inodes.\n");
        //closeDisk(mountedDisk);
        mountedDisk = -1;
        return TFS_ERROR_DISK_OPEN;
    }
    // Verify magic number
    if (temp->magicNumber != MAGIC_NUMBER) {
        printf("tfs_mount: Invalid magic number: %d, expected %d.\n", temp->magicNumber, MAGIC_NUMBER);
        //closeDisk(mountedDisk);
        mountedDisk = -1;
        return TFS_ERROR_BAD_MAGIC;
    }
    free(temp);
    printf("Successfully initialized FS.\n");
    return TFS_SUCCESS;
}

int tfs_unmount(void) {
    if (mountedDisk < 0) {
        printf("tfs_unmount: No disk is currently mounted.\n");
        return -1;
    }

    //closeDisk(mountedDisk);
    mountedDisk = -1;

    return 0;
}

int tfs_openFile(char* name) {
    if (mountedDisk == -1) {
        printf("tfs_openFile: No disk mounted error.\n");
        return TFS_ERROR_NO_DISK_MOUNTED;
    }
    if (current_fd != -1) {
        printf("tfs_openFile: a file is already open, current_fd: %d\n", 
            current_fd);
        return TFS_ERROR_FILE_OPEN;
    }
    if (strnlen(name, MAX_FILENAME_LENGTH) == 0) {
       return TFS_ERROR_INVALID_PARAMETER;
    }
    int inodeIndex = -1;
    int i = 0;

    for (i = 0; i < MAX_FILES; i++) {
       // Check if file already exists
       Inode * tmpIdx = NULL;
       if (!(tmpIdx = getLL(inodes, i))) {
          return TFS_ERROR_INVALID_FD;
       }
       if (!tmpIdx) {
          return TFS_ERROR_INVALID_FD;
       }
       if (strcmp(tmpIdx->name, name) == 0) {
          return i;
       }
    }
    for (i = 0; i < MAX_FILES; i++) {
       Inode * tmpIdx = NULL;
       if (!(tmpIdx = getLL(inodes, i))) {
          return TFS_ERROR_INVALID_FD;
       }
       if (!tmpIdx) {
          return TFS_ERROR_INVALID_FD;
       }
       if ((tmpIdx->size) == 0) {
          //found free inode
          inodeIndex = i;
          if (!(activeInode = getLL(inodes, i))) {
             return TFS_ERROR_INVALID_FD;
          }
       }
    }
    
    if (inodeIndex == -1 || activeInode == NULL) {
        printf("tfs_openFile: Max files reached error.\n");
        return TFS_ERROR_MAX_FILES_REACHED;
    }
    
    // Create new file
    strncpy(activeInode->name, name, MAX_FILENAME_LENGTH);
    activeInode->size = 0;
    // Assign single FileExtentBlock to new file
    /*
    FileExtentBlock * newBlock = NULL;
    newBlock = (FileExtentBlock *)malloc(sizeof(FileExtentBlock));
    if (!newBlock) {
       return TFS_ERROR_BAD_ALLOC;
    }
    if (insertIntoLL((activeInode->dataBlocks), newBlock) < 0) {
       printf("tfs_openFile: Could not insert new FEBlock into inode list.\n");
       return TFS_ERROR_BAD_ALLOC;
    }
    */   
    
    // Write updated inode to disk
    Inode * updated = NULL;
    if (!(updated = getLL(inodes, inodeIndex))) {
        return TFS_ERROR_INVALID_FD;
    }
    if (writeBlock(mountedDisk, inodeIndex + 1, updated) < 0) {
        printf("tfs_openFile: Disk write error.\n");
        return TFS_ERROR_DISK_WRITE;
    }
    lastBlockIndex++;
    filePointer = 0;
    //TODO: this is wasteful, for loop probably not needed.
    current_fd = inodeIndex;
    return current_fd;
}

int tfs_closeFile(fileDescriptor FD) {
    if (mountedDisk == -1) {
        printf("tfs_closeFile: No disk mounted error.\n");
        return TFS_ERROR_NO_DISK_MOUNTED;
    }
    
    if (FD < 0 || FD >= MAX_FILES) {
        printf("tfs_closeFile: Invalid FD error.\n");
        return TFS_ERROR_INVALID_FD;
    }
    // retrieve associated inode
    Inode * inode = NULL;
    if (!(inode = getLL(inodes, FD))) {
        return TFS_ERROR_INVALID_FD;
    }
    //handled in freeBlock()
    //int allocatedBlocks = sizeOfLL(inode->dataBlocks);


    // clear associated data with inode
    freeBlock(inode);

    // restore free blocks
    // DONE IN FREEBLOCK()
    //freeBlocks += allocatedBlocks;
    filePointer = 0;
    current_fd = -1;
    return TFS_SUCCESS;
}

int tfs_writeFile(fileDescriptor FD, char* buffer, int size) {
    if (mountedDisk == -1) {
        printf("tfs_writeFile: No disk mounted error.\n");
        return TFS_ERROR_NO_DISK_MOUNTED;
    }
    
    if (FD < 0 || FD >= MAX_FILES) {
        printf("tfs_writeFile: Invalid FD error.\n");
        return TFS_ERROR_INVALID_FD;
    }
    if (buffer == NULL) {
        printf("tfs_writeFile: buffer is null.\n");
        return TFS_ERROR_INVALID_PARAMETER;
    }
    if (freeBlocks == 0) {
        printf("tfs_writeFile: disk is full.\n");
        return TFS_ERROR_DISK_FULL;
    }
    Inode * inode = getLL(inodes, FD);
    if (inode == NULL) {
        printf("tf_writeFile: Inode not found.\n");
        return TFS_ERROR_INVALID_FD;
    }
    //clear inode contents if they exist
    //freeBlock(inode); //CHECK
    
    int fileExtentSize = (BLOCKSIZE-(18+sizeof(int)));
    int blocks = size / fileExtentSize;
    if (size % fileExtentSize > 0) {
       //allocate one more block
       blocks++;
    }
    int remaining = blocks;
    char * current = buffer;
    // write new content
    while (remaining > 0 && freeBlocks > 0) {
       FileExtentBlock * newBlock = NULL;
       newBlock = (FileExtentBlock *)malloc(sizeof(FileExtentBlock));
       if (newBlock == NULL) {
          return TFS_ERROR_BAD_ALLOC;
       }
       int copySize = min(remaining, sizeof(newBlock->data));
       memcpy(newBlock->data, current, copySize);
       newBlock->code = 3;
       newBlock->magicNumber = MAGIC_NUMBER;
       newBlock->inodePtr = inode;
       newBlock->nextBlockPtr = NULL;
       //write newBlock
       if ((writeBlock(mountedDisk, lastBlockIndex, newBlock)) < 0) {
           printf("tfs_writeFile: Could not write to block. LBI: %d\n", lastBlockIndex);
           return TFS_ERROR_DISK_WRITE;
       }
       lastBlockIndex++;
       insertIntoLL(inode->dataBlocks, newBlock);
       current += copySize;
       remaining -= copySize;
       // we should remove a free block from the LL
       FreeBlock * fb = NULL;
       fb = removeTailLL(freeBlocksList);
       if (fb == NULL) {
          printf("tfs_writeFile: Could not retrieve FreeBlock to use.\n");
          return TFS_ERROR_BAD_ALLOC;
       }
       //free(fb); // goodbye
       freeBlocks--;
    }
    if (remaining > 0 && freeBlocks == 0) {
       printf("tfs_writeFile: could not complete write, disk full. \n");
       return TFS_ERROR_DISK_FULL;
    }
    // write inode to disk
    inode->size = blocks;
    if ((writeBlock(mountedDisk, inode->idx, inode)) < 0) {
       printf("tfs_writeFile: could not write inode to disk.\n");
       return TFS_ERROR_DISK_WRITE;
    }
    filePointer = 0;
    printf("Successfully written to file. Congratulations.\n");
    return TFS_SUCCESS;
}


int tfs_deleteFile(fileDescriptor FD) {
    if (mountedDisk == -1) {
        printf("tfs_deleteFile: No disk mounted error\n");
        return TFS_ERROR_NO_DISK_MOUNTED;
    }
    
    if (FD < 0 || FD >= MAX_FILES) {
        printf("tfs_deleteFile: Invalid FD error.\n");
        return TFS_ERROR_INVALID_FD;
    }

    Inode * inode = NULL;
    inode = getLL(inodes, FD);
    freeBlock(inode);
    // freeBlock() transforms all FEB into FB within inode list
    // and writes updated inode back to disk
    
    return TFS_SUCCESS;
}

int tfs_readByte(fileDescriptor FD, char* buffer) {
    if (mountedDisk == -1) {
        printf("tfs_readByte: No disk is mounted.\n");
        return TFS_ERROR_NO_DISK_MOUNTED;
    }

    if (FD < 0 || FD >= MAX_FILES) {
        printf("tfs_readByte: Invalid file descriptor.\n");
        return TFS_ERROR_INVALID_FD;
    }

    if (buffer == NULL) {
        printf("tfs_readByte: Buffer is NULL.\n");
        return TFS_ERROR_INVALID_PARAMETER;
    }

    Inode * inode = getLL(inodes, FD);
    if (inode == NULL) {
        printf("tfs_readByte: Inode not found.\n");
        return TFS_ERROR_INVALID_FD;
    }

    // Check if file pointer is within file size
    // Account for firstBlockLocation
    if (filePointer >= (firstBlockLocation + ((inode->size) * BLOCKSIZE))) {
        printf("tfs_readByte: File pointer is beyond the end of the file: %d\n", 
               filePointer);
        return TFS_ERROR_END_OF_FILE;
    }
    // offset for superblocks and inodes
    int filePointerStart = firstBlockLocation + filePointer;

    // Calculate which block the file pointer is in
    // in terms of the size of FEB's
    //targetNumBlocks = (int)ceil((double)(ftePtr->curByteOffset + 1) / BLOCK_DATA_SIZE);
    //int blockIndex = filePointer / (256 - (18 + sizeof(int)));
    //int offsetInBlock = filePointer % (256 - (18 + sizeof(int)));

    //ALL blocks are 256 bytes, this should get us to the correct location
    int blockIndex = (int)ceil(filePointerStart / BLOCKSIZE);
    // and this should get us to the byte we need, after we retrieve the block from disk
    //int blockOffset = (int)ceil((double)filePointerStart % BLOCKSIZE);

    //accomodate for first (18+sizeof(int)) bytes held by FEB struct
    int blockOffset = (int)ceil(filePointerStart % BLOCKSIZE) + (18 + sizeof(int));

    //TECHNICALLY don't even need to read from disk as we have all data stored in the
    //associated FEB, but I assume this is how its supposed to be implemented

    // create temporary FEB to hold read data
    FileExtentBlock * temp = NULL;
    temp = (FileExtentBlock *)malloc(sizeof(FileExtentBlock));

    //printf("attempting to read block on mountedDisk: %d\n", mountedDisk);
    if (readBlock(mountedDisk, blockIndex, temp) < 0) {
       printf("tfs_readByte: Could not read from disk into buffer.\n");
       return TFS_ERROR_DISK_READ;
    }
    // Read the byte from the block and copy to buffer
    *buffer = temp->data[blockOffset];


    // Get the corresponding FileExtentBlock
    //FileExtentBlock *feb = (FileExtentBlock *)getLL(inode->dataBlocks, blockIndex);
   


    //if (!feb) {
    //    printf("tfs_readByte: Could not retrieve File Extent Block.\n");
    //    return TFS_ERROR_BAD_ALLOC;
    //}

    // Increment the file pointer
    filePointer++;

    return TFS_SUCCESS;
}



/*
int tfs_seek(fileDescriptor FD, int offset) {
    if (mountedDisk == -1) {
        printf("tfs_seek: No disk mounted.\n");
        return TFS_ERROR_NO_DISK_MOUNTED;
    }

    if (FD < 0 || FD >= MAX_FILES) {
        printf("tfs_seek: Invalid fd.\n");
        return TFS_ERROR_INVALID_FD;
    }

    Inode* inode = getLL(inodes, FD);
    if (inode == NULL) {
        printf("tfs_seek: Inode not found.\n");
        return TFS_ERROR_INVALID_FD;
    }

    if (offset < 0 || offset > inode->size) {
        printf("tfs_seek: Invalid Offset: %d\n",offset);
        return TFS_ERROR_INVALID_OFFSET;
    }

    inode->filePointer = offset;

    return TFS_SUCCESS;
}


/*
void test_tfs_mkfs() {
    printf("Testing tfs_mkfs...\n");
    int ret = tfs_mkfs("test_disk.bin", 102400);
    if (ret == TFS_SUCCESS) {
        printf("tfs_mkfs test passed!\n");
    } else {
        printf("tfs_mkfs test failed with error code: %d\n", ret);
    }
}

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

// Main function
/*
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
