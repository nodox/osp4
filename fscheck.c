#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "fs.h"

#define T_DIR  1   // Directory
#define T_FILE 2   // File
#define T_DEV  3   // Device

struct superblock super_block;
char block_buff[BSIZE];

// helper methods for using bitmap data in the main method

// bitmasks
const uchar bitmasks[8] = {0b00000001, 0b00000010, 0b00000100, 0b00001000, 0b00010000, 0b00100000, 0b01000000, 0b10000000};

// get bit from bitmap by index
uchar get_bit_from_bitmap(uchar* bitmap, int i) {
    int loc = i >> 3;
    int off = i & 7;
    return (bitmap[loc] & bitmasks[off]) >> off;
}

// set value in bitmap by index
void set_bit_to_bitmap(uchar* bitmap, int i) {
    int loc = i >> 3;
    int off = i & 7;
    bitmap[loc] |= bitmasks[off];
}

// test valid and set value in bitmap by index, return location
uchar test_and_set_bit_in_bitmap(uchar* bitmap, int i) {
    int loc = i >> 3;
    int off = i & 7;
    uchar prev = (bitmap[loc] & bitmasks[off]) >> off;
    bitmap[loc] |= bitmasks[off];
    return prev;
}
// end helper methods

int main(int argc, char *argv[]) 
{
  // check if file img arg provided
  if(argc < 2){
    fprintf(stderr, "%s\n", "ERROR: image not found");
    exit(1);
  }

  // read in fs.img
  FILE* fsimg = fopen(argv[1], "rb");

  if(fsimg == NULL){
    fprintf(stderr, "%s\n", "ERROR: image not found");
    exit(1);
  }
  else{
    // seek past empty boot block
    fseek(fsimg, BSIZE, SEEK_SET);
  }

  // read in and store superblock data
  fread(block_buff, 1, BSIZE, fsimg);
  memcpy(&super_block, block_buff, sizeof(struct superblock));

  // seek to inodes
  fseek(fsimg, BSIZE*super_block.inodestart, SEEK_CUR);

  int numinodes = (super_block.ninodes + IPB - 1) / IPB;
  struct dinode inodes[IPB * numinodes];
  // read in and store inode data
  for (int i = 0; i < numinodes; ++i)
  {
    fread(block_buff, 1, BSIZE, fsimg);
    memcpy(&inodes[i * IPB], block_buff, BSIZE);
  }
  // seek to bitmap
  fseek(fsimg, BSIZE*(super_block.bmapstart), SEEK_SET);

  int numbitmap = (super_block.size + BPB - 1) / BPB;
  uchar bitmap[BSIZE * numbitmap];
  // read in and store bitmap data
  for (int i = 0; i < numbitmap; ++i)
  {
    fread(block_buff, 1, BSIZE, fsimg);
    memcpy(bitmap, block_buff, BSIZE);
  }

  // generate temp bitmap for comparisons to actual bitmap
  uchar temp_bitmap[BSIZE * numbitmap];
  memset(temp_bitmap, 0, sizeof(temp_bitmap));
  // set initial blocks
  set_bit_to_bitmap(temp_bitmap, 0);
  set_bit_to_bitmap(temp_bitmap, 1);

  // base of data
  int data = numbitmap + numinodes + 3;

  // set inode blocks
  for (int i = 2; i <= numinodes + 2; ++i)
  {
    set_bit_to_bitmap(temp_bitmap, i);
  }

  for (int i = 2 + numinodes; i < numinodes + numbitmap + 2; ++i)
  {
    set_bit_to_bitmap(temp_bitmap, i);
  }

  // store parent, children and reference counts for later structure checks
  ushort parents[super_block.ninodes];
  memset(parents, 0, sizeof(parents));

  ushort children[super_block.ninodes];
  memset(children, 0, sizeof(children));
  children[1] = 1;

  ushort refs[super_block.ninodes];
  memset(refs, 0, sizeof(refs));

  
  // recurse fs starting at root, storing data for later structure checks
  printf("%s\n", "starting first pass of fs for links and presence");
  for (int i = 0; i < super_block.ninodes; ++i)
  {
    // inode has no type/data
    if (inodes[i].type == 0)
    {
      break;
    }
    // bad type error
    if (inodes[i].type > T_DEV)
    {
      fprintf(stderr, "%s\n", "ERROR: bad inode");
      exit(1);
    }
    // no root directory at expected location
    if (i == 1 && inodes[i].type != T_DIR)
    {
      fprintf(stderr, "%s\n", "ERROR: root directory does not exist");
      exit(1);
    }
    // check direct links
    for (int j = 0; j < NDIRECT; ++j)
    {
      if (inodes[i].addrs[j] == 0)
      {
        continue;
      }
      if (inodes[i].addrs[j] > super_block.size || inodes[i].addrs[j] < data)
      {
        fprintf(stderr, "%s\n", "ERROR: bad address in inode");
        exit(1);
      }
      if (test_and_set_bit_in_bitmap(temp_bitmap, inodes[i].addrs[j]) != 0)
      {
        fprintf(stderr, "%s\n", "ERROR: address used more than once");
        exit(1);
      }
      if (inodes[i].type == T_DIR)
      {
        fseek(fsimg, BSIZE * inodes[i].addrs[j], SEEK_SET);
        fread(block_buff, 1, BSIZE, fsimg);
        struct dirent* dirent = (struct dirent*)block_buff;

        int k = 0;
        if (j == 0)
        {
          if (i == 1 || dirent[1].inum != 1)
          {
            fprintf(stderr, "%s\n", "ERROR: root directory does not exist");
            exit(1);
          }
          if (strcmp(".", dirent[0].name) != 0 || strcmp("..", dirent[1].name) != 0)
          {
            fprintf(stderr, "%s\n", "ERROR: directory not properly formatted");
            exit(1);
          }
          parents[i] = dirent[1].inum;
          k = 2;
        }
        for (; k < BSIZE / sizeof(struct dirent); ++i)
        {
          if (dirent[k].inum != 0)
          {
            refs[dirent[k].inum] += 1;
            children[dirent[k].inum] = i;
          }
        }
      }
    }

    if (inodes[i].addrs[NDIRECT] == 0){
      continue;
    }
    if (inodes[i].addrs[NDIRECT] > super_block.size || inodes[i].addrs[NDIRECT] < data)
    {
      fprintf(stderr, "%s\n", "ERROR: bad address in inode");
      exit(1);
    }
    if (test_and_set_bit_in_bitmap(temp_bitmap, inodes[i].addrs[NDIRECT]) != 0)
    {
      fprintf(stderr, "%s\n", "ERROR: address used more than once");
      exit(1);
    }

    // check indirect links
    int indirect_links[NINDIRECT];
    fseek(fsimg, BSIZE * inodes[i].addrs[NDIRECT], SEEK_SET);
    fread(indirect_links, 1, BSIZE, fsimg);
    for (int j = 0; j < NINDIRECT; ++i)
    {
      if (indirect_links[j] == 0)
      {
        break;
      }
      if (indirect_links[j] > super_block.size || indirect_links[j] < data)
      {
        fprintf(stderr, "%s\n", "ERROR: bad address in inode");
        exit(1);
      }
      if (test_and_set_bit_in_bitmap(temp_bitmap, indirect_links[j]) != 0) {
          fprintf(stderr, "%s\n", "ERROR: address used more than once");
          exit(1);
      }
      if (inodes[i].type == T_DIR)
      {
        fseek(fsimg, BSIZE * inodes[i].addrs[j], SEEK_SET);
        fread(block_buff, 1, BSIZE, fsimg);
        struct dirent* dirent = (struct dirent*)block_buff;

        for (int k = 0; k < BSIZE / sizeof(struct dirent); ++k)
        {
          if (dirent[k].inum != 0)
          {
            refs[dirent[k].inum] += 1;
            children[dirent[k].inum] = 1;
          }
        }
      }
    }
  }
  printf("%s\n", "finished first pass of fs for links and presence");

  // compare developed bitmap to given bitmap
  printf("%s\n", "starting bitmap check/comparison");
  for (int i = 0; i < numbitmap; ++i)
  {
    if (temp_bitmap[i] == bitmap[i])
    {
      continue;
    }
    if (temp_bitmap[i] > bitmap[i])
    {
      printf("at: %d\n", i);
      fprintf(stderr, "%s\n", "ERROR: address used by inode but marked free in bitmap");
      exit(1);
    }
    fprintf(stderr, "%s\n", "ERROR: bitmap marks block in use but it is not in use");
    exit(1);
  }

  printf("%s\n", "finished bitmap check/comparison");

  // check structure and compare inodes array to observed objects (files, directories etc.)
  printf("%s\n", "starting structure check and inode comparison");
  for (int i = 0; i < (super_block.ninodes-sizeof(double)); ++i)
  {
    if (inodes[i].type == 0)
    {
      if(refs[i] > 0){
        fprintf(stderr, "%s\n", "ERROR: inode referred to in directory but marked free");
        exit(1);
      }
    }
    else {
      if(refs[i] == 0){
        fprintf(stderr, "%s\n", "ERROR: inode marked use but not found in a directory");
        exit(1);
      }
      if (inodes[i].type == T_DIR)
      {
        if (refs[i] > 1)
        {
          fprintf(stderr, "%s\n", "ERROR: directory appears more than once in file system");
          exit(1);
        }
        if (parents[i] != children[i])
        {
          fprintf(stderr, "%s\n", "ERROR: parent directory mismatch");
          exit(1);
        }
      }
      if (inodes[i].type == T_FILE)
      {
        if (inodes[i].type != refs[i])
        {
          fprintf(stderr, "%s\n", "ERROR: bad reference count for file");
          exit(1);
        }
      }
    }
  }

  printf("%s\n", "finished structure check and inode comparison");

  printf("%s\n", "fscheck done");

  fclose(fsimg);
  return 0;
}