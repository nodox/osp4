#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "fs.h"

struct superblock super_block;
char block_buff[BSIZE];

int main(int argc, char *argv[]) 
{
  // check if file img arg provided
  if(argc < 2){
    fprintf(stderr, "%s\n", "image not found");
    exit(1);
  }

  // read in fs.img
  FILE* fsimg = fopen(argv[1], "rb");

  if(fsimg == NULL){
    fprintf(stderr, "%s\n", "image not found");
    exit(1);
  }
  else{
    // seek past empty block
    fseek(fsimg, BSIZE, SEEK_SET);
  }

  // store superblock data
  fread(block_buff, 1, BSIZE, fsimg);
  memcpy(&super_block, block_buff, sizeof(struct superblock));

  printf("%d\n", super_block.ninodes);

  fclose(fsimg);
  return 0;
}