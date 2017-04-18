#define NUM_INODES
#define FREE
#define TAKEN

int NUM_INODES;
int NUM_BLOCKS;

void init(){
    struct block_info* bi = get_lru_block(1);
    struct fs_header header = *(struct fs_header*)block_info->data;
    NUM_INODES = fs_header.num_inodes;
    NUM_BLOCKS = fs_header.num_blocks;

    short* free_inodes = (short*)malloc(NUM_INODES * sizeof(short));
    short* free_blocks = (short*)malloc(NUM_BLOCKS * sizeof(short));
   
    free_blocks[0] = TAKEN;  // boot block is taken
    free_inodes[0] = TAKEN;  // fs_header inode is also taken

    int i;

    // loop through all inodes
    for(i = 1; i < NUM_INODES; i++){
	struct inode* current_inode = get_lru_inode(i)->inode_val;
	if(current_inode->type == INODE_FREE){
	    free_blocks[i] = TAKEN;

	    int j = 0;
	    // loop through direct blocks
	    while(j < NUM_DIRECT && j * BLOCKSIZE < current_inode->size){
		free_blocks[current_inode->direct[j]] = TAKEN;
		j++;		    
	    }

	    // if file still has more blocks, explore indirect block as well
	    if(j * BLOCKSIZE < current_inode->size){
		int* indirect_block = (int*)(get_lru_block(current_inode->indirect)->data);
		while(j * BLOCKSIZE < current_inode->size){
		    free_blocks[indirect_block[j - NUM_DIRECT]] == TAKEN;
		    j++;
		}
	    }
	}
    }
}
