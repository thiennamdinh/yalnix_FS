#ifndef _yfs_h
#define _yfs_h

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#include <comp421/yalnix.h>
#include <comp421/hardware.h>
#include <comp421/filesystem.h>
#include <comp421/iolib.h>

#define FILE_SYSTEM 1

#define MESSAGE_SIZE 32

#define CODE_OPEN 1
#define CODE_CLOSE 2
#define CODE_CREATE 3
#define CODE_READ 4
#define CODE_WRITE 5
#define CODE_SEEK 6
#define CODE_LINK 7
#define CODE_UNLINK 8
#define CODE_SYMLINK 9
#define CODE_READLINK 10
#define CODE_MKDIR 11
#define CODE_RMDIR 12
#define CODE_CHDIR 13
#define CODE_STAT 14
#define CODE_SYNC 15
#define CODE_SHUTDOWN 16

#define FREE 0
#define TAKEN 1

#define SIZE 100
#define CACHESIZE 32 // Needs to be modified to another number.

int NUM_INODES;
int NUM_BLOCKS;

short* free_inodes;
short* free_blocks;

int NUM_DIRS_PER_BLOCK = BLOCKSIZE / sizeof(struct dir_entry);


//Block variables:
int current_blockcache_number;
struct block_info {
    int dirty;
    int block_number;
    struct block_info *next;
    struct block_info *prev;
    char data[BLOCKSIZE];
};

struct block_wrap {
   int key;
   struct block_info* block_data;
};

struct block_info* block_front;
struct block_info* block_rear;

struct block_wrap* block_hashtable[SIZE]; 
struct block_wrap* default_block_wrap;
struct block_info* default_block_info;

//Inode variable
int current_inodecache_number;

struct inode_info {
    int dirty;
    int inode_number;
    struct inode_info *next;
    struct inode_info *prev;
    struct inode *inode_val; //From the filesystem.h
};

struct inode_wrap {
   int key;
   struct inode_info* inode_data;
};
struct inode_info* inode_front;
struct inode_info* inode_rear;

struct inode_wrap* inode_hashtable[SIZE]; 
struct inode_wrap* default_inode_wrap;
struct inode_info* default_inode_info;

int FSOpen(char *pathname, short current_dir);
int FSCreate(char *pathname, short current_dir);
int FSRead(void *buf, int size, short inode, int position);
int FSWrite(void *buf, int size, short inode, int position);
int FSSeek(short inode);
int FSLink(char *oldname, char *newname, short current_dir);
int FSUnlink(char *pathname, short current_dir);
int FSSymLink(char *oldname, char *newname, short current_dir);
int FSReadLink(char *pathname, char *buf, int len, short current_dir);
int FSMkDir(char *pathname, short current_dir);
int FSRmDir(char *pathname, short current_dir);
int FSChDir(char *pathname, short current_dir);
int FSStat(char *pathname, struct Stat* statbuf, short current_dir);
int FSSync(void);
int FSShutdown(void);
int Redirect_Call(char* msg, int pid);

void set_lru_inode(int inode_num, struct inode_info* input_inode);

#endif /*!_yfs_h*/
