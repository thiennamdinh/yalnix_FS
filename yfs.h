#ifndef _yfs_h
#define _yfs_h

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

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

int NUM_INODES;
int NUM_BLOCKS;

short* free_inodes;
short* free_blocks;


/* commenting out because compiler is complainting about redefinition
//Block
typedef struct block_wrap {
    int dirty;
    int block_number;
    char data[BLOCKSIZE];
} block_wrap;

typedef struct block_hash_table {
    int key;
    block_wrap *val;
} block_hash_table;

//Inode
typedef struct inode_wrap {
    int dirty;
    int inode_number;
    struct inode_wrap *prev;
    struct inode_wrap *next;
    struct inode data;
} inode_wrap;


typedef struct inode_hash_table{
    int key;
    inode_wrap *val;
    struct inode_hash_table *next;
    struct inode_hash_table *prev;
} inode_hash_table;
*/
#endif /*!_yfs_h*/


int FSOpen(char *pathname);
int FSCreate(char *pathname);
int FSRead(void *buf, int size, short inode, int position);
int FSWrite(void *buf, int size, short inode, int position);
int FSSeek(short inode);
int FSLink(char *oldname, char *newname);
int FSUnlink(char *pathname);
int FSSymLink(char *oldname, char *newname);
int FSReadLink(char *pathname, char *buf, int len);
int FSMkDir(char *pathname);
int FSRmDir(char *pathname);
int FSChDir(char *pathname);
int FSStat(char *pathname, struct Stat* statbuf);
int FSSync(void);
int FSShutdown(void);
int Redirect_Call(char* msg, int pid);
