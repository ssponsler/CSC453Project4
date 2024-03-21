
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "libTinyFS.h"

int main(int argc, char *argv[]) {

   int fd = -1;
   int longFD = -1;
   int i = 0;

   int countTest = 0;

   char *testString1 = "test test test test test test test test test test test test";
   char *testString2 = "Steven Sinofsky became the president of the Windows division in July 2009. His first heavily-involved projects included Windows Live Wave 3 and Internet Explorer 8. Sinofsky and Jon DeVaan also headed the development of the next major version of Windows to come after Windows Vista, Windows 7. Sinofsky's philosophy on Windows 7 was to not make any promises about the product or even discuss anything about the product until Microsoft was sure that it felt like a quality product. This was a radical departure from Microsoft's typical way of handling in-development versions of Windows, which was to publicly share all plans and details about it early in the development cycle. Sinofsky also refrained from labeling versions of Windows";


   /* Test 1: open file, write to file, close file*/

   countTest = 0;

   printf("Test 1: open file, write to file, close file\n");

   if ((fd = tfs_openFile("testfile")) == -1) {
      printf("tfs_openFile failed\n");
      exit(-1);
   }
   countTest++;

   // write some bytes
   if (tfs_writeFile(fd, testString1, 0) == -1) {
      puts("tfs_write failed");
      exit(-1);
   }
   countTest++;

   if (tfs_closeFile(fd) == -1) {
      printf("tfs_closeFile failed\n");
      exit(-1);
   }
   countTest++;

   if (countTest == 3)
      printf("Test 1: Passed\n");
   else
      printf("Test 1: Failed\n");

   /* Test 2: Open file, write to file, read file, close file */

   countTest = 0;

   printf("Test 2: Open file, write to file, read file, close file\n");

   if ((fd = tfs_openFile("testfile")) == -1) {
      printf("tfs_openFile failed\n");
      exit(-1);
   }
   countTest++;

   if (tfs_writeFile(fd, testString1, 0) == -1) {
      puts("tfs_write failed");
      exit(-1);
   }
   countTest++;

   char result1[100] = "";
   i = 0;
   while(tfs_readByte(fd, &(result1[i])) != -1) {
      if (result1[i] == '\0') {
         break;
      }
      i++;
   }

   if (strcmp(result1, testString1) == 0) {
      printf("File Contents:\n[%s]\n", result1);
      countTest++;
   }

   // close the file
   if (tfs_closeFile(fd) == -1) {
      printf("tfs_closeFile failed\n");
      exit(-1);
   }
   countTest++;

   if (countTest == 4)
      printf("Test 2: Passed\n");
   else
      printf("Test 2: Failed\n");


   /* Test 3: Open file, read file, close file */

   countTest = 0;

   printf("Test 3: Open file, read file, close file\n");

   if ((fd = tfs_openFile("testfile")) == -1) {
      printf("tfs_openFile failed\n");
      exit(-1);
   }
   countTest++;

   // reading to the end of the file
   char result2[100] = "";
   i = 0;
   while(tfs_readByte(fd, &(result2[i])) != -1) {
      if (result2[i] == '\0') {
         break;
      }
      i++;
   }
   if (strcmp(result2, testString1) == 0) {
      printf("File Contents:\n[%s]\n", result1);
      countTest++;
   }

   if (tfs_closeFile(fd) == -1) {
      printf("tfs_closeFile failed\n");
      exit(-1);
   }
   countTest++;

   if (countTest == 3)
      printf("Test 3: Passed\n");
   else
      printf("Test 3: Failed\n");


   /* Test 4: Open file, seek past the end (err), read and close*/

   countTest = 0;

   printf("Test 4: Open file, seek past the end (err), read and close\n");

   //open the file again
   if ((fd = tfs_openFile("testfile")) == -1) {
      printf("tfs_openFile failed\n");
      exit(-1);
   }
   countTest++;

   // seek past the end of the file
   if (tfs_seek(fd, 100) == -1) {
      printf("tfs_seek failed\n");
      exit(-1);
   }
   countTest++;

   char result4[100] = "";
   i = 0;
   while(tfs_readByte(fd, &(result4[i])) != -1) {
      if (result4[i] == '\0') {
         break;
      }
      i++;
   }
   if (strcmp(result4, "") == 0) {
      printf("File Contents:\n[%s]\n", result1);
      countTest++;
   }

   // close the file
   if (tfs_closeFile(fd) == -1) {
      printf("tfs_closeFile failed\n");
      exit(-1);
   }
   countTest++;

   if (countTest == 4)
      printf("Test 4: Passed\n");
   else
      printf("Test 4: Failed\n");

   /*-----------*/ 

   countTest = 0;

   printf("Test 5: list creation time of testfile\n");

   // open file
   if ((fd = tfs_openFile("testfile")) == -1) {
      printf("tfs_openFile failed\n");
      exit(-1);
   }
   countTest++;

   time_t time = -1;
   time = tfs_getTimeStamp(fd);
   if (time != (time_t)-1) {
      countTest++;
   }

   printf("testfile creation time: %llu\n", (long long unsigned int)time);

   // close the file
   if (tfs_closeFile(fd) == -1) {
      printf("tfs_closeFile failed\n");
      exit(-1);
   }
   countTest++;

   if (countTest == 3)
      printf("Test 5: Passed\n");
   else
      printf("Test 5: Failed\n");

   /*-----------*/ 

   countTest = 0;

   printf("Test 6: Open file, seek 10 bytes into the file, write to seeked location, read file and close it\n");

   //open the file again
   if ((fd = tfs_openFile("testfile")) == -1) {
      printf("tfs_openFile failed\n");
      exit(-1);
   }
   countTest++;

   // seek into the file
   if (tfs_seek(fd, 10) == -1) {
      printf("tfs_seek failed\n");
      exit(-1);
   }
   countTest++;

   // write to file
   if (tfs_writeFile(fd, testString1, 0) == -1) {
      printf("tfs_openFile failed\n");
      exit(-1);
   }
   countTest++;

   // read to end of the file
   char fileResult17[100] = "";
   i = 0;
   while(tfs_readByte(fd, &(fileResult17[i])) != -1) {
      if (fileResult17[i] == '\0') {
         break;
      }
      i++;
   }
   if (strcmp(fileResult17, testString1) == 0) {
      //printf("File Contents:\n[%s]\n", result1);
      countTest++;
   }

   // close the file
   if (tfs_closeFile(fd) == -1) {
      printf("tfs_closeFile failed\n");
      exit(-1);
   }
   countTest++;

   if (countTest == 5)
      printf("Test 6: Passed\n");
   else
      printf("Test 6: Failed\n");

   return 0;
}
