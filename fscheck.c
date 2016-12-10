#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>

//http://codewiki.wikidot.com/c:system-calls:fstat


#define BSIZE 512  // block size 
#define NDIRECT 12
#define NINDIRECT (BSIZE / sizeof(uint))
#define MAXFILE (NDIRECT + NINDIRECT)
          

// File system super block                                                      
struct superblock {
  uint size;         // Size of file system image (blocks)                      
  uint nblocks;      // Number of data blocks                                   
  uint ninodes;      // Number of inodes.
  uint nlog;         // Number of log blocks
  uint logstart;     // Block number of first log block
  uint inodestart;   // Block number of first inode block
  uint bmapstart;    // Block number of first free map block                                       
};

// On-disk inode structure
struct dinode {
  short type;           // File type
  short major;          // Major device number (T_DEV only)
  short minor;          // Minor device number (T_DEV only)
  short nlink;          // Number of links to inode in file system
  uint size;            // Size of file (bytes)
  uint addrs[NDIRECT+1];   // Data block addresses
};

// Inodes per block.                                                            
#define IPB           (BSIZE / sizeof(struct dinode))

// Block containing inode i                                                     
#define IBLOCK(i)     ((i) / IPB + 2)

// Bitmap bits per block                                                        
#define BPB           (BSIZE*8)

// Block containing bit for block b                 
#define BBLOCK(b, ninodes) (b/BPB + (ninodes)/IPB + 3)
//address offset of given block from fsptr
//#define ABLOCK(blockNumber, ninodes) ((1+1+ninodes/IPB+1+1+blockNumber)*BSIZE)
#define ABLOCK(blockNumber, ninodes) blockNumber*BSIZE

// Directory is a file containing a sequence of dirent structures.    
#define DIRSIZ 14

struct dirent {
  ushort inum;
  char name[DIRSIZ];
};

void *fsptr; 
struct superblock *sb;

// struct stat {
//                dev_t     st_dev;     /* ID of device containing file */
//                ino_t     st_ino;     /* inode number */
//                mode_t    st_mode;    /* protection */
//                nlink_t   st_nlink;   /* number of hard links */
//                uid_t     st_uid;     /* user ID of owner */
//                gid_t     st_gid;     /* group ID of owner */
//                dev_t     st_rdev;     // device ID (if special file) 
//                off_t     st_size;    /* total size, in bytes */
//                blksize_t st_blksize; /* blocksize for file system I/O */
//                blkcnt_t  st_blocks;  /* number of 512B blocks allocated */
//                time_t    st_atime;   /* time of last access */
//                time_t    st_mtime;   /* time of last modification */
//                time_t    st_ctime;   /* time of last status change */
//            };



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
  printf("size: %d nblocks: %d ninodes: %d\n", sb->size, sb->nblocks, sb->ninodes);

  while(1) {
    printf("%c\n", (char*)fsptr++);
  }



  


  
}