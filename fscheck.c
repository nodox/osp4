#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>

//http://codewiki.wikidot.com/c:system-calls:fstat


#define BSIZE 512  // block size           

// File system super block                                                      
struct superblock {
  uint size;         // Size of file system image (blocks)                      
  uint nblocks;      // Number of data blocks                                   
  uint ninodes;      // Number of inodes.                                       
};

void *fsptr; 
struct superblock *sb;





int main(int argc, char *argv[]) 
{

  // buffer for stat
  struct stat fileStat;


  //open fs.img
  int fd = open(argv[1], O_RDWR);
  fstat(fd, &fileStat);
  int fsize = fileStat.st_size; //get fs size

  printf("File Size: \t\t%d bytes\n",fileStat.st_size);
  printf("Number of Links: \t%d\n",fileStat.st_nlink);
  printf("File inode: \t\t%d\n",fileStat.st_ino);

  fsptr = mmap(NULL, fsize, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);

  sb = (struct superblock*)((void*)fsptr + BSIZE); //super block starts at second block
  printf("size:%d nblocks:%d ninodes%d\n", sb->size, sb->nblocks, sb->ninodes);
  


  
}