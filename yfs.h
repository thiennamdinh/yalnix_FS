//Block
typedef struct block_wrap {
    int dirty;
    int block_number;
    char data[BLOCKSIZE];
} block_cache;

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

