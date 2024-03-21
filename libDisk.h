
#ifndef __LIB_DISK_H_
#define __LIB_DISK_H_

#define DISK_NAME "tinyFSDisk"
#define DISK_SIZE 10240

int openDisk(char *filename, int nBytes);

int readBlock(int disk, int blockIdx, void *block);

int writeBlock(int disk, int blockIdx, void *block);

int closeDisk(int disk);

int diskExists(char *filename);

int getDiskSize(int disk);

#endif // __LIBDISK_H_
