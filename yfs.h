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

//Block
typedef struct block_wrap {
    int dirty;
    int block_number;
    struct block_wrap *prev;
    struct block_wrap *next;
    char data[BLOCKSIZE];
} block_wrap;

typedef struct block_hash_table {
    int key;
    block_wrap *val;
    struct block_hash_table *prev;
    struct block_hash_table *next;
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

#endif /*!_yfs_h*/
