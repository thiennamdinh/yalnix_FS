#include "yfs.h"

int calculate_inode_to_block_number(int inode_number) ;
struct block_info* read_block_from_disk(int block_num);
int sync();

void Print() {
   struct block_info* temp = block_front;
   while(temp != NULL) {
       temp = temp->next;
   }
   free(temp);
}

void init() {
   current_blockcache_number = 0;
   block_front = NULL;
   block_rear = NULL;
   default_block_wrap = (struct block_wrap*) malloc(sizeof(struct block_wrap));
   default_block_wrap->key = -1;

   default_block_info = (struct block_info*) malloc(sizeof(struct block_info));
   default_block_info->block_number = -1;

   current_inodecache_number = 0;
   inode_front = NULL;
   inode_rear = NULL;
   default_inode_wrap = (struct inode_wrap*) malloc(sizeof(struct inode_wrap));
   default_inode_wrap->key = -1;

   default_inode_info = (struct inode_info*) malloc(sizeof(struct inode_info));
   default_inode_info->inode_number = -1;

   default_inode_wrap->inode_data = default_inode_info;
}

int calculate_inode_to_block_number(int inode_number) {
  return 1 + (inode_number / (BLOCKSIZE / INODESIZE));
}



int generate_hash_code(int key) {
   //Hash code generator.
   return key % SIZE;
}

struct block_wrap *get_block(int key) {
   //Obtains the hash code.
   int hash_index = generate_hash_code(key);  
	
   while(block_hashtable[hash_index] != NULL) {
	
      if(block_hashtable[hash_index]->key == key){
         return block_hashtable[hash_index]; 
      }
			
      ++hash_index;
		
      hash_index %= SIZE;
   }        
	
   return NULL;        
}

void put_block_to_hashtable(int key, struct block_info* data_input) {

   struct block_wrap *item = (struct block_wrap*) malloc(sizeof(struct block_wrap));
   item->block_data = data_input;
   item->key = key;

   int hash_index = generate_hash_code(key);

   while(block_hashtable[hash_index] != NULL && block_hashtable[hash_index]->key != -1) {
      ++hash_index;
		
      hash_index %= SIZE;
   }
	
   block_hashtable[hash_index] = item;
}

struct block_wrap* remove_block_from_hashtable(int block_num) {

   int hash_index = generate_hash_code(block_num);

   while(block_hashtable[hash_index] != NULL) {
	
      if(block_hashtable[hash_index]->key == block_num) {
         struct block_wrap* temp = block_hashtable[hash_index]; 
			
         block_hashtable[hash_index] = default_block_wrap; 

         return temp;
      }
		
      ++hash_index;
		
      hash_index %= SIZE;
   }      
	
   return NULL;        
}

void enqueue_block(struct block_info * x) {
   //Puts to end.
   if(block_front == NULL && block_rear == NULL){
      block_front = block_rear = x;
      block_front->prev = NULL;
      block_rear->next = NULL;
      return;
   }
   block_rear->next = x;
   x->prev = block_rear;
   block_rear = x;
   block_rear->next = NULL;
   block_front->prev = NULL;
}

void dequeue_block() {
   //Eliminate the block_front;
   if(block_front == NULL) {
      printf("Queue is Empty\n");
      return;
   }
   if(block_front == block_rear) {
      block_front = block_rear = NULL;
   }else {
      block_front->next->prev = NULL;
      block_front = block_front->next;
      block_front->prev = NULL;
   }

}

void remove_queue_block(struct block_info * x) {
   if(x->prev != NULL && x->next != NULL){
      x->prev->next = x->next;
      x->next->prev = x->prev;
      x->prev = NULL;
      x->next = NULL;
   }else if(x->prev != NULL && x->next == NULL) {
      block_rear = block_rear->prev;
      block_rear->next = NULL;
      x->prev = NULL;
      x->next = NULL;
   }else if(x->prev == NULL && x->next != NULL) {
      dequeue_block();
      x->prev = NULL;
      x->next = NULL;
   }
}


struct block_info* get_lru_block(int block_num) {
    struct block_wrap* result = get_block(block_num);
    if(result == NULL) {
	return default_block_info;
    }else{
	//Recently used.
	remove_queue_block(result->block_data);
	enqueue_block(result->block_data);
	// put_to_block_front(result);

	return result->block_data;
    }
}

int sync() {
    //Write everything back to the disk. Syncs everything.
    struct inode_info* tmp_inode = inode_front;
    struct block_info* tmp_block = block_front;

    while(tmp_inode != NULL) {
	int inode_number = tmp_inode->inode_number;
	//if(tmp_inode->dirty == 1) {
	    //The value is dirty.
	    int block_num_to_write = calculate_inode_to_block_number(inode_number);
	    struct block_info *tmp = read_block_from_disk(block_num_to_write);
	    memcpy((void*)(tmp->data + (inode_number - (BLOCKSIZE/INODESIZE) * (block_num_to_write - 1)) * INODESIZE), (void*)(tmp_inode->inode_val), INODESIZE);
	    tmp->dirty = 1;
	    tmp_inode->dirty = 0;
	    //}
	tmp_inode = tmp_inode->next;
    }

    while(tmp_block != NULL) {
	//if(tmp_block->dirty == 1) {
	    //The block is dirty, so write it back.
	    int sig = WriteSector(tmp_block->block_number, (void*)(tmp_block->data));
	    if(sig == 0) {
		tmp_block->dirty = 0;

	    }else{
		printf("An error is generated when doing WriteSector.\n");
		return -1;
	    }
	    //Marks the current block to not dirty.
	    tmp_block->dirty = 0;
	    //}
	tmp_block = tmp_block->next;
    }
    return 0;
}


void evict_block(){
   //Test whether there is a key to be evict_blocked.
   if(current_blockcache_number >= BLOCK_CACHESIZE) {
      int to_be_removed_key = block_front->block_number;
      //Here should be another method sync to write inode back to the disk.
      sync();
      if(block_front->dirty == 1) {
         int sig = WriteSector(block_front->block_number, (void*)(block_front->data));
         if(sig == 0) {
         	printf("An error is generated when doing WriteSector.\n");
         }
      }
      dequeue_block();
      remove_block_from_hashtable(to_be_removed_key);
      //Decrement the current block cache number by 1.
      current_blockcache_number--;
   }
}

void set_lru_block(int block_num, struct block_info* input_block) {
   if(get_block(block_num) == NULL) {
      // printf("Key not found\n");
      //Determines whether a key needs to be removed.
      evict_block();
      enqueue_block(input_block);
      put_block_to_hashtable(block_num, input_block);
      current_blockcache_number++;
      return;
   }else{

      remove_queue_block(get_block(block_num)->block_data);
      enqueue_block(input_block);
      put_block_to_hashtable(block_num, input_block);

      return;
   }

}


//Inode functions

struct inode_wrap *get_inode(int key) {
   //Obtains the hash code.
   int hash_index = generate_hash_code(key);  
   
   while(inode_hashtable[hash_index] != NULL) {
   
      if(inode_hashtable[hash_index]->key == key){
         return inode_hashtable[hash_index]; 
      }
         
      ++hash_index;
      
      hash_index %= SIZE;
   }        
   
   return NULL;        
}

void put_inode_to_hashtable(int key, struct inode_info* data_input) {

   struct inode_wrap *item = (struct inode_wrap*) malloc(sizeof(struct inode_wrap));
   item->inode_data = data_input;
   item->key = key;

   int hash_index = generate_hash_code(key);

   while(inode_hashtable[hash_index] != NULL && inode_hashtable[hash_index]->key != -1) {
      ++hash_index;
      
      hash_index %= SIZE;
   }
   
   inode_hashtable[hash_index] = item;
}

struct inode_wrap* remove_inode_from_hashtable(int inode_num) {

   int hash_index = generate_hash_code(inode_num);

   while(inode_hashtable[hash_index] != NULL) {
   
      if(inode_hashtable[hash_index]->key == inode_num) {
         struct inode_wrap* temp = inode_hashtable[hash_index]; 
         
         inode_hashtable[hash_index] = default_inode_wrap; 

         return temp;
      }
      
      ++hash_index;
      
      hash_index %= SIZE;
   }      
   
   return NULL;        
}

void enqueue_inode(struct inode_info * x) {
   //Puts to end.
   if(inode_front == NULL && inode_rear == NULL){
      inode_front = inode_rear = x;
      inode_front->prev = NULL;
      inode_rear->next = NULL;
      return;
   }
   inode_rear->next = x;
   x->prev = inode_rear;
   inode_rear = x;
   inode_rear->next = NULL;
}


void dequeue_inode() {
   //Eliminate the inode_front;
   if(inode_front == NULL) {
      printf("Queue is Empty\n");
      return;
   }
   if(inode_front == inode_rear) {
      inode_front = inode_rear = NULL;
   }else {
      inode_front->next->prev = NULL;
      inode_front = inode_front->next;
      inode_front->prev = NULL;
   }

}

void remove_queue_inode(struct inode_info * x) {
   if(x->prev != NULL && x->next != NULL){
      x->prev->next = x->next;
      x->next->prev = x->prev;
      x->next = NULL;
      x->prev = NULL;
   }else if(x->prev != NULL && x->next == NULL) {
      inode_rear = inode_rear->prev;
      inode_rear->next = NULL;
      x->next = NULL;
      x->prev = NULL;
   }else if(x->prev == NULL && x->next != NULL) {
      dequeue_inode();
      x->next = NULL;
      x->prev = NULL;
   }
}


struct inode_info* get_lru_inode(int inode_num) {
   struct inode_wrap* result = get_inode(inode_num);
   if(result == NULL) {
      return default_inode_info;
   }else{
      //Recently used.
      remove_queue_inode(result->inode_data);
      enqueue_inode(result->inode_data);
      // put_to_inode_front(result);

      return result->inode_data;
   }
}

struct block_info* read_block_from_disk(int block_num) {
	struct block_info* result = get_lru_block(block_num);
	if(result->block_number == -1) {
	    //Reads from the disk.
	    result = (struct block_info*)malloc(sizeof(struct block_info));
	    ReadSector(block_num, (void*)(result->data));
	    //Sets the dirty to not dirty
	    result->dirty = 0;
	    result->block_number = block_num;
	    set_lru_block(block_num, result);
	    //To obtain the block_info.
	    return get_lru_block(block_num);

	}else{
		return result;
	}
}

struct inode_info* read_inode_from_disk(int inode_num) {
    struct inode_info* result = get_lru_inode(inode_num);
    if(result->inode_number == -1) {
	int block_num = calculate_inode_to_block_number(inode_num);
	struct block_info* tmp = get_lru_block(block_num);
	if(tmp->block_number == -1) {
	    tmp = read_block_from_disk(block_num);
	}
	result = (struct inode_info*) malloc(sizeof(struct inode_info));
	result->inode_number = inode_num;
	result->dirty = 0;
	result->inode_val = (struct inode*)malloc(sizeof(struct inode));

	memcpy((result->inode_val), tmp->data+(inode_num-(block_num-1)*(BLOCKSIZE/INODESIZE)) * INODESIZE, INODESIZE);
	set_lru_inode(inode_num, result);
    }
    return result;
}


void set_lru_inode(int inode_num, struct inode_info* input_inode) {
   if(get_inode(inode_num) == NULL) {
      //Determines whether a key needs to be removed.

   	   //evicts.
	   //Test whether there is a key to be evict_inodeed.
	   if(current_inodecache_number >= INODE_CACHESIZE) {
	      int to_be_removed_key = inode_front->inode_number;
	      //Here should be another method sync to write inode back to the disk.

	      //Remove the key from the hashtable
	      remove_inode_from_hashtable(to_be_removed_key);
	      //Should call sync.
	      sync();

	      if(inode_front->dirty == 1) {
	      	int block_num_to_write = calculate_inode_to_block_number(to_be_removed_key);
	      	struct block_info *tmp = read_block_from_disk(block_num_to_write);
	      	int index = inode_num - (BLOCKSIZE/INODESIZE) * (block_num_to_write - 1);
	      	memcpy((void*)(tmp->data + index * INODESIZE), (void*)(inode_front->inode_val), INODESIZE);
	      	tmp->dirty = 1;
	      }

	      dequeue_inode();
	      remove_inode_from_hashtable(to_be_removed_key);



	      //Decrement the current block cache number by 1.
	      current_inodecache_number--;
	   }
      enqueue_inode(input_inode);
      put_inode_to_hashtable(inode_num, input_inode);
      current_inodecache_number++;
      return;
   }else{


      remove_queue_inode(get_inode(inode_num)->inode_data);
      enqueue_inode(input_inode);
      put_inode_to_hashtable(inode_num, input_inode);
      return;
   }
}

int check_dir(int dir_inum, char* filename){
    struct inode_info* info = read_inode_from_disk(dir_inum);
    if(info->inode_number == ERROR){
	fprintf(stderr, "ERROR: could not read directory\n");
	return ERROR;
    }

    struct dir_entry entry;
    int num_entries = info->inode_val->size / sizeof(struct dir_entry);
    
    int i;
    for(i = 0; i < num_entries; i++){
	if(FSRead((void*)&entry, sizeof(entry), dir_inum, i * sizeof(struct dir_entry)) == ERROR){
	    return ERROR;
	}
	if(strlen(filename) < DIRNAMELEN && strcmp(filename, entry.name) == 0){
	    return entry.inum;
	}
	//TODO need to take care of case where filename == DIRNAMELEN
    }
    return 0;
}

int convert_pathname_to_inode_number(char *pathname, int proc_inum) {
    if(pathname == NULL ) {
	return proc_inum;
    }
    int cur_inode;
    char node_name[DIRNAMELEN + 1];
    memset(node_name,'\0',DIRNAMELEN + 1);
    int sym_link_cnt = 0;

    //Means that it is an absolute pathname, so it starts with ROOTINODE.
    if(pathname[0] == '/') {
	cur_inode = ROOTINODE;
    }
    else{
	cur_inode = proc_inum;
    }
    
    //Kills all the initial slashes.
    while(*pathname == '/'){
	pathname++;
    }

    while(strlen(pathname) != 0) {
        int len_path = strlen(pathname);
	memset(node_name,'\0',DIRNAMELEN + 1);
    	
	while(len_path > 0 && *pathname == '/') {
	    pathname++;
	    len_path--;
	}
	int i = 0;
	while(len_path > 0 && *pathname != '/') {
	    node_name[i] = *pathname;
	    i++;
	    pathname++;
	    len_path--;
	}
        
	struct inode_info* n;
        int sub_inum = check_dir(cur_inode, node_name);
	if (sub_inum<=0) {
	    fprintf(stderr, "ERROR: failed to parse given path\n");
	    return ERROR;
	}

	n = read_inode_from_disk(sub_inum);
	//If the sub inode is a symbolic link.
	if(n->inode_val->type == INODE_SYMLINK) {
	    if(sym_link_cnt >= MAXSYMLINKS) {
		return ERROR;
	    }else{
		int data_size = n->inode_val->size;
		char* new_pathname = (char*) malloc(sizeof(char) * data_size + len_path + 1);
		struct block_info* b = read_block_from_disk(n->inode_val->direct[0]);
		memcpy(new_pathname, b->data, data_size);
          
		strcat(new_pathname, "///");
		strcat(new_pathname, pathname);
		pathname = new_pathname;

		sym_link_cnt++;
	    }
	}
	cur_inode = sub_inum;
    }
    return cur_inode;
}

void print_dir(int dir_inum){
    
    struct inode_info* info = read_inode_from_disk(dir_inum);
    if(info->inode_number == ERROR){
	fprintf(stderr, "ERROR: could not read directory\n");
    }

    struct dir_entry entry;
    int num_entries = info->inode_val->size / sizeof(struct dir_entry);
    
    printf("\n---- directory %d ----\n", dir_inum);

    int i;
    for(i = 0; i < num_entries; i++){
	FSRead((void*)&entry, sizeof(entry), dir_inum, i * sizeof(struct dir_entry));
       	printf("%d - %s\n", entry.inum, entry.name); 
    }
    printf("----------------------\n\n");
}


/*
int check_dir(int direct_inum, char* filename) {
    struct inode_info* dir = read_inode_from_disk(direct_inum);
    printf("---- directory %d ----- \n", direct_inum);
    if(dir->inode_val->type!= INODE_DIRECTORY) {
	fprintf(stderr, "ERROR: This is not a directory.\n");
	return ERROR;
    }

    int i, block_num;
    // check through direct blocks
    for(i=0;i<NUM_DIRECT;i++) {
	if(i * BLOCKSIZE + 1 > dir->inode_val->size) {
	    break;
	}else{
	    block_num = dir->inode_val->direct[i];
	    struct block_info* b = read_block_from_disk(block_num);
	    struct dir_entry *d = (struct dir_entry*)(b->data);
	    int j;
	    for(j=0;j<NUM_DIRS_PER_BLOCK;j++) {
		printf("file name %s\n", d[j].name);
		if(d[j].inum<=0){
		    continue;
		}
		if(strcmp(d[j].name, filename) == 0) {
		    return d[j].inum;
		}
	    }
	}
    }
    // check indirect blocks
    if(dir->inode_val->size > NUM_DIRECT * BLOCKSIZE) {
	block_num = dir->inode_val->indirect;
	struct block_info* b = read_block_from_disk(block_num);
	int j;
	struct block_info* tmp;
	for(j=0;j<BLOCKSIZE;j+=4) {
	    block_num = *(int*)(b->data+j);
	    if(block_num!=0) {
		tmp = read_block_from_disk(block_num);
		struct dir_entry *d = (struct dir_entry*)(tmp->data);
		int k;
		for(k=0;k<NUM_DIRS_PER_BLOCK;k++) {
		    if(d[j].inum<=0) continue;
		    if(strcmp(d[j].name, filename) == 0) {
			return d[j].inum;
		    }
		}
	    }else{
		break;
	    }
	}
    }
    return 0;
}
*/
int get_free_inode(){
    int i ;
    for ( i = 0; i < NUM_INODES; ++i)
	{
	    /* code */
	    if(free_inodes[i] == FREE) {
		return i;
	    }
	}
    return -1;
}
int get_free_block() {
    int i ;
    for ( i = 0; i < NUM_BLOCKS; ++i)
	{
	    /* code */
	    if(free_blocks[i] == FREE) {
		return i;
	    }
	}
    return -1;
}

// allocate space for the file_inode to hold up to "newsize" data
int grow_file(struct inode* file_inode, int newsize){
    if(newsize < file_inode->size){
	return 0;
    }

    // round filesize up to the next blocksize
    int current = ((file_inode->size + (BLOCKSIZE-1)) / BLOCKSIZE) * BLOCKSIZE;
    // fill up direct blocks first
    if(current < BLOCKSIZE * NUM_DIRECT){
        while(current < BLOCKSIZE * NUM_DIRECT && current < newsize){
	    // assign a new block in direct
	    int free_block = get_free_block();
	    if(free_block == -1) {
		return -1;
	    }
	    file_inode->direct[current / BLOCKSIZE] = free_block;
	    free_blocks[free_block] = TAKEN;
	    current += BLOCKSIZE;

	}
    }
    
    // if direct blocks not enough, then access indirect blocks
    if(current < newsize){
	int big_block_num = file_inode->indirect;
	struct block_info * block_indirect = read_block_from_disk(big_block_num);
	block_indirect->dirty = 1;
	int * int_array = (int*)(block_indirect->data);
	while(current < BLOCKSIZE * NUM_DIRECT && current < newsize ) {
	    int i;
	    for (i = 0; i < BLOCKSIZE; ++i){
		int free_block = get_free_block();
		if(free_block == -1) {
		    return ERROR;
		}
		free_blocks[free_block] = TAKEN;
		int_array[i] = free_block;
		free_blocks[free_block] = TAKEN;
	    }
	    current += BLOCKSIZE;
	}
    }
    file_inode->size = newsize;
    return 0;
}

// returns a pointer to the data starting at "position" in the file described by "file_inode"
char* get_data_at_position(struct inode* file_inode, int position){

    if(position > file_inode->size){
	fprintf(stderr, "ERROR: trying to read past size of file");
	return NULL;
    }

    int file_block_num = position / BLOCKSIZE;

    // if position is within direct blocks
    if(file_block_num < NUM_DIRECT){
	struct block_info* info = read_block_from_disk(file_inode->direct[file_block_num]);
	return info->data + position % BLOCKSIZE;
    }

    // if position is within indirect blocks
    struct block_info* indirect_info = read_block_from_disk(file_inode->indirect);
    int target_num = (long)(indirect_info->data) + (file_block_num - NUM_DIRECT);
    
    struct block_info* target_info = read_block_from_disk(target_num);
    return target_info->data + position % BLOCKSIZE;
}

int add_directory_entry(short dir_inum, struct dir_entry new_entry){
    struct inode_info* dir_info = read_inode_from_disk(dir_inum);
    dir_info->dirty = 1;

    if(dir_info->inode_number == -1 || dir_info->inode_val->type != INODE_DIRECTORY){
	fprintf(stderr, "ERROR: not a valid directory inode number\n");
    }

    int dir_size = dir_info->inode_val->size;
    int position = 0;
    struct dir_entry old_entry;
    
    // look for a blank entry in the middle of the directory first and overwrite with new entry
    while(position < dir_size){
	FSRead(&old_entry, sizeof(old_entry), dir_inum, position);
	if(old_entry.inum == 0){
	    return FSWrite(&new_entry, sizeof(new_entry), dir_inum, position);
	}
	position += sizeof(old_entry);
    }
    // if none available, write new entry at the end of the file
    return FSWrite((void*)&new_entry, sizeof(new_entry), dir_inum, position);
}

int get_parent_inum(char* pathname, short current_dir){
    // parse backwards and look for last '/'
    int i;
    for(i = strlen(pathname) - 1; i >= 0; i--){
	if(pathname[i] == '/'){
	    break;
	}
    }
    int parent_path_len = i + 1;
    
    // attempt to get inode of parent directory
    char* parent_path = (char*)malloc(parent_path_len + 1);
    memcpy(parent_path, pathname, parent_path_len);
    parent_path[parent_path_len] = '\0';

    // strip the trailing '/' unless the parent directory is root
    if(!(parent_path_len == 1 && parent_path[0] == '/')){
	parent_path[parent_path_len-1] = '\0';
    }

    short parent_inum = convert_pathname_to_inode_number(parent_path, current_dir);
    free(parent_path);

    if(parent_inum == ERROR){
	fprintf(stderr, "ERROR: failed to obtain path to parent directory\n");
	return ERROR;
    }
    return parent_inum;
}

// takes a pathname and returns a pointer to the name of the last file in the path
// note that the pointer points to a section of the original parameter
char* get_filename(char* pathname){
    int i;
    for(i = strlen(pathname) - 1; i >= 0; i--){
	if(pathname[i] == '/'){
	    return pathname + i + 1;
	}
    }
    return pathname;
}

// creates a new file under the given parent directory
int create_file(char* filename, short parent_inum, int type){
    short file_inum;

    if(check_dir(parent_inum, filename) == ERROR){
	fprintf(stderr, "ERROR: file already exists\n");
	return ERROR;
    }

    // allocate new inode for file
    short i;
    for(i = 0; i < NUM_INODES; i++){
	if(free_inodes[i] == FREE){
	    file_inum = i;
	    break;
	}
    }
    if(file_inum == NUM_INODES){
	fprintf(stderr, "ERROR: no more inodes left for new file\n");
	return ERROR;
    }

    free_inodes[i] = TAKEN;
    
    struct inode_info* file_info = read_inode_from_disk(file_inum);
    struct inode* file_inode = file_info->inode_val;
    file_inode->type = type;
    file_inode->nlink = 1;
    file_inode->reuse++;
    file_inode->size = 0;
    file_info->dirty = 1;
    
    //TODO: correct way to rewrite inode to disk?

    // create and populate new directory entry
    struct dir_entry entry;
    entry.inum = file_inum;
    int filename_len = strlen(filename);
    if(filename_len > DIRNAMELEN)
	filename_len = DIRNAMELEN;
    memcpy(entry.name, filename, filename_len);

    if(add_directory_entry(parent_inum, entry) == ERROR){
	// undo prior process to create inode for the file
	free_inodes[file_inum] = FREE;
	file_inode->type = INODE_FREE;
	file_inode->nlink = 0;
	file_inode->reuse--;
	
	fprintf(stderr, "ERROR: failed to add file to directory\n");
	return ERROR;
    }

    return file_inum;
}

void init_free(){
    
    struct inode_info* i_info = read_inode_from_disk(0);
    struct fs_header* header = (struct fs_header*)(i_info->inode_val);

    NUM_INODES = header->num_inodes;
    NUM_BLOCKS = header->num_blocks;

    free_inodes = (short*)malloc(NUM_INODES * sizeof(short));
    free_blocks = (short*)malloc(NUM_BLOCKS * sizeof(short));
     
    int i;
    for (i = 0; i < NUM_BLOCKS; ++i){
	free_blocks[i] = FREE;
    }
    for (i = 0; i < NUM_INODES; ++i){
	free_inodes[i] = FREE;
    }
    free_inodes[0] = TAKEN;  // fs_header inode is taken   
    free_inodes[1] = TAKEN;  // root inode taken   
    free_blocks[0] = TAKEN;  // boot block is taken

    // inode blocks are taken
    for(i = 1; i < 1 + ((NUM_INODES + 1) * INODESIZE) / BLOCKSIZE; i++){
	free_blocks[i] = TAKEN;
    }

    // loop through all inodes
    for(i = 1; i < NUM_INODES + 1; i++){
	struct inode* current_inode = read_inode_from_disk(i)->inode_val;
	
	if(current_inode->type != INODE_FREE){	    
	    int j = 0;
	    // loop through direct blocks
	    while(j < NUM_DIRECT && j * BLOCKSIZE < current_inode->size){
		free_blocks[current_inode->direct[j]] = TAKEN;
		j++;		    
	    }

	    // if file still has more blocks, explore indirect block as well
	    if(j * BLOCKSIZE < current_inode->size){
		int* indirect_block = (int*)(read_block_from_disk(current_inode->indirect)->data);
		while(j * BLOCKSIZE < current_inode->size){
		    free_blocks[indirect_block[j - NUM_DIRECT]] = TAKEN;
		    j++;
		}

	    }
	}
    }
}

int FSOpen(char *pathname, short current_dir){
    return convert_pathname_to_inode_number(pathname, current_dir);
}

int FSCreate(char *pathname, short current_dir){
    short parent_inum = get_parent_inum(pathname, current_dir);
    char* filename = get_filename(pathname);
    return create_file(filename, parent_inum, INODE_REGULAR);
}

int FSRead(void *buf, int size, short inode, int position){
    struct inode_info* info = read_inode_from_disk(inode);
    if(info->inode_number == -1){
	fprintf(stderr, "ERROR: not a valid inode number\n");
	return ERROR;
    }
    struct inode* file_inode = info->inode_val;

    int offset = 0;
    // keep reading while buf is not full and we have not reached the end of the file
    while(offset < size && position + offset < file_inode->size){
	Pause();
        char* data = get_data_at_position(file_inode, position + offset);
	
	// readable size is min of space left in current block and size left in buffer
	int readable_size = BLOCKSIZE;
	if((position + offset) % BLOCKSIZE != 0)
	    readable_size = (position + offset) % BLOCKSIZE;
	if(size - offset < readable_size)
	    readable_size = size - offset;
	if(file_inode->size - (position + offset) < readable_size)
	    readable_size = file_inode->size - (position + offset);
	
	memcpy(buf + offset, data, readable_size);
	offset += readable_size;
    }

    return 0;
}

int FSWrite(void *buf, int size, short inum, int position){
    struct inode_info* info = read_inode_from_disk(inum);
    info->dirty = 1;
    if(info->inode_number == -1){
	fprintf(stderr, "ERROR: not a valid inode number\n");
	return ERROR;
    }
    struct inode* file_inode = info->inode_val;
    // if writing past the size of the current file then we need to expand it first
    if(grow_file(file_inode, position + size) == ERROR){
	fprintf(stderr, "ERROR: failed to grow file in write operation\n");
	return ERROR;
    }

    int offset = 0;
    // write over existing data first
 
    while(offset < size && position + offset < file_inode->size){
	char* data = get_data_at_position(file_inode, position + offset);
	// writeable size is min of blocksize, space left in buf, and space left in file
	int writeable_size = BLOCKSIZE;
	if((position + offset) % BLOCKSIZE != 0)
	    writeable_size = (position + offset) % BLOCKSIZE;
	if(size - offset < writeable_size)
	    writeable_size = size - offset;

	memcpy(data, buf + offset, writeable_size);
	offset += writeable_size;
    }

    info->dirty = 1;
    return 0;
}

// This just returns the size of a given file... it's only necessary for a particular case of
// seek in the iolib
int FSSeek(short inode){
    struct inode_info* info = read_inode_from_disk(inode);
    if(info->inode_number == -1){
	fprintf(stderr, "ERROR: not a valid inode number\n");
	return ERROR;
    }
    return info->inode_val->size;
}

int FSLink(char *oldname, char *newname, short current_dir){
    return 0;
}

int FSUnlink(char *pathname, short current_dir){
    return 0;
}

int FSSymLink(char *oldname, char *newname, short current_dir){
    short parent_inum = get_parent_inum(newname, current_dir);
    char* filename = get_filename(newname);
    short inum = create_file(filename, parent_inum, INODE_SYMLINK);
    if(inum == ERROR)
	return ERROR;

    int result = FSWrite((void*)oldname, strlen(oldname), inum, 0);
    return result;
}

int FSReadLink(char *pathname, char *buf, int len, short current_dir){
    return 0;
}

int FSMkDir(char *pathname, short current_dir){
    short parent_inum = get_parent_inum(pathname, current_dir);
    char* filename = get_filename(pathname);
    short inum = create_file(filename, parent_inum, INODE_DIRECTORY);
    if(inum == ERROR)
	return ERROR;
    struct dir_entry dot;
    struct dir_entry dotdot;
    
    
    char* strdot = ".";
    memcpy(dot.name, strdot, strlen(strdot));
    dot.name[strlen(strdot)] = '\0';
    dot.inum = inum;

    char* strdotdot = "..";
    memcpy(dotdot.name, strdotdot, strlen(strdotdot));
    dotdot.name[strlen(strdotdot)] = '\0';
    dotdot.inum = parent_inum;

    add_directory_entry(inum, dot);
    add_directory_entry(inum, dotdot);
 
    return 0;
}

int FSRmDir(char *pathname, short current_dir){
    return 0;
}

int FSChDir(char *pathname, short current_dir){
    return 0;
}

int FSStat(char *pathname, struct Stat* statbuf, short current_dir){
    short inum = convert_pathname_to_inode_number(pathname, current_dir);
    struct inode_info* info = read_inode_from_disk(inum);

    if(info->inode_number == -1){
	fprintf(stderr, "ERROR: pathname not valid\n");
	return ERROR;
    }
    statbuf->inum = info->inode_number;
    statbuf->type = info->inode_val->type;
    statbuf->size = info->inode_val->size;
    statbuf->nlink = info->inode_val->nlink;

    return 0;
}

int FSSync(void){
    return sync();
}

int FSShutdown(void){
    sync();
    Exit(0);
}

int Redirect_Call(char* msg, int pid){
#pragma GCC diagnostic push /*ignore unavoidable gcc warning caused by unconventional casting */
#pragma GCC diagnostic ignored "-Wint-to-pointer-cast"
    
    char* current = msg;
    uint8_t code = (uint8_t)(msg[0]);
    int result;
        
    switch(code){
	case CODE_OPEN:{
	    char* upathname; int upathname_size; short ucurrent_dir;
	    memcpy(&upathname, current += sizeof(code), sizeof(upathname));
	    memcpy(&upathname_size, current += sizeof(upathname), sizeof(upathname_size));
	    memcpy(&ucurrent_dir, current += sizeof(upathname_size), sizeof(ucurrent_dir));

	    char* pathname = (char*)malloc(upathname_size + 1);
	    CopyFrom(pid, pathname, upathname, upathname_size + 1);
	    result = FSOpen(pathname, ucurrent_dir);
	    free(pathname);
	    break;
	}
	case CODE_CREATE:{
	    char* upathname; int upathname_size; short ucurrent_dir;
	    memcpy(&upathname, current += sizeof(code), sizeof(upathname));
	    memcpy(&upathname_size, current += sizeof(upathname), sizeof(upathname_size));
	    memcpy(&ucurrent_dir, current += sizeof(upathname_size), sizeof(ucurrent_dir));

	    char* pathname = (char*)malloc(upathname_size + 1);
	    CopyFrom(pid, pathname, upathname, upathname_size + 1);
	    result = FSCreate(pathname, ucurrent_dir);
	    free(pathname);
	    break;
	}
	case CODE_READ:{
	    char* ubuf; int usize; short uinode; int uposition;
	    memcpy(&ubuf, current += sizeof(code), sizeof(ubuf));
	    memcpy(&usize, current += sizeof(ubuf), sizeof(usize));
	    memcpy(&uinode, current += sizeof(usize), sizeof(uinode));
	    memcpy(&uposition, current += sizeof(uinode), sizeof(uposition));
 
	    char* buf = malloc(usize + 1);
	    result = FSRead(buf, usize, uinode, uposition);
	    CopyTo(pid, ubuf, buf, usize + 1);
	    free(buf);
	    break;
	}
	case CODE_WRITE:{
	    char* ubuf; int usize; short uinode; int uposition;
	    memcpy(&ubuf, current += sizeof(code), sizeof(ubuf));
	    memcpy(&usize, current += sizeof(ubuf), sizeof(usize));
	    memcpy(&uinode, current += sizeof(usize), sizeof(uinode));
	    memcpy(&uposition, current += sizeof(uinode), sizeof(uposition));

	    char* buf = malloc(usize + 1);
	    CopyFrom(pid, buf, ubuf, usize + 1);
	    result = FSWrite(buf, usize, uinode, uposition);
	    free(buf);
	    break;
	}
	case CODE_SEEK:{
	    short inode;	    
	    memcpy(&inode, current += sizeof(code), sizeof(inode));
	    result = FSSeek(inode);
	    break;
	}
	case CODE_LINK:{
	    char* uoldname; int uoldname_size; char* unewname; int unewname_size;
	    short ucurrent_dir;
	    memcpy(&uoldname, current += sizeof(code), sizeof(uoldname));
	    memcpy(&uoldname_size, current += sizeof(uoldname), sizeof(uoldname_size));
	    memcpy(&unewname, current += sizeof(unewname_size), sizeof(unewname));
	    memcpy(&unewname_size, current += sizeof(unewname), sizeof(unewname_size));
	    memcpy(&ucurrent_dir, current += sizeof(unewname_size), sizeof(ucurrent_dir));

	    char* oldname = malloc(uoldname_size + 1);
	    char* newname = malloc(unewname_size + 1);
	    CopyFrom(pid, oldname, uoldname, uoldname_size + 1);
	    CopyFrom(pid, newname, unewname, unewname_size + 1);
	    result = FSLink(oldname, newname, ucurrent_dir);
	    break;
	}
	case CODE_UNLINK:{
	    char* upathname; int upathname_size; short ucurrent_dir;
	    memcpy(&upathname, current += sizeof(code), sizeof(upathname));
	    memcpy(&upathname_size, current += sizeof(upathname), sizeof(upathname_size));
	    memcpy(&ucurrent_dir, current += sizeof(upathname_size), sizeof(ucurrent_dir));

	    char* pathname = (char*)malloc(upathname_size + 1);
	    CopyFrom(pid, pathname, upathname, upathname_size + 1);
	    result = FSUnlink(pathname, ucurrent_dir);
	    free(pathname);
	    break;
	}
	case CODE_SYMLINK:{
	    char* uoldname; int uoldname_size; char* unewname; int unewname_size;
	    short ucurrent_dir;
	    memcpy(&uoldname, current += sizeof(code), sizeof(uoldname));
	    memcpy(&uoldname_size, current += sizeof(uoldname), sizeof(uoldname_size));
	    memcpy(&unewname, current += sizeof(unewname_size), sizeof(unewname));
	    memcpy(&unewname_size, current += sizeof(unewname), sizeof(unewname_size));
	    memcpy(&ucurrent_dir, current += sizeof(unewname_size), sizeof(ucurrent_dir));

	    char* oldname = malloc(uoldname_size + 1);
	    char* newname = malloc(unewname_size + 1);
	    CopyFrom(pid, oldname, uoldname, uoldname_size + 1);
	    CopyFrom(pid, newname, unewname, unewname_size + 1);
	    result = FSSymLink(oldname, newname, ucurrent_dir);
	    break;
	}
	case CODE_READLINK:{
	    char* upathname; int upathname_size; char* ubuf; int ulen; short ucurrent_dir;
	    memcpy(&upathname, current += sizeof(code), sizeof(upathname));
	    memcpy(&upathname_size, current += sizeof(upathname), sizeof(upathname_size));
	    memcpy(&ubuf, current += sizeof(upathname_size), sizeof(ubuf));
	    memcpy(&ulen, current += sizeof(ubuf), sizeof(ulen));
	    memcpy(&ucurrent_dir, current += sizeof(ulen), sizeof(ucurrent_dir));

	    char* pathname = malloc(upathname_size + 1);
	    char* buf = malloc(ulen + 1);
	    CopyFrom(pid, pathname, upathname, upathname_size + 1);
	    result = FSReadLink(pathname, buf, ulen, ucurrent_dir);
	    CopyTo(pid, ubuf, buf, ulen + 1);
	    free(pathname);
	    free(buf);
	    break;
	}
	case CODE_MKDIR:{
	    char* upathname; int upathname_size; short ucurrent_dir;
	    memcpy(&upathname, current += sizeof(code), sizeof(upathname));
	    memcpy(&upathname_size, current += sizeof(upathname), sizeof(upathname_size));
	    memcpy(&ucurrent_dir, current += sizeof(upathname_size), sizeof(ucurrent_dir));

	    char* pathname = (char*)malloc(upathname_size + 1);
	    CopyFrom(pid, pathname, upathname, upathname_size + 1);
	    result = FSMkDir(pathname, ucurrent_dir);
	    free(pathname);
	    break;
	}
	case CODE_RMDIR:{
	    char* upathname; int upathname_size; short ucurrent_dir;
	    memcpy(&upathname, current += sizeof(code), sizeof(upathname));
	    memcpy(&upathname_size, current += sizeof(upathname), sizeof(upathname_size));
	    memcpy(&ucurrent_dir, current += sizeof(upathname_size), sizeof(ucurrent_dir));

	    char* pathname = (char*)malloc(upathname_size + 1);
	    CopyFrom(pid, pathname, upathname, upathname_size + 1);
	    result = FSRmDir(pathname, ucurrent_dir);
	    free(pathname);
	    break;
	}
	case CODE_CHDIR:{
	    char* upathname; int upathname_size; short ucurrent_dir;
	    memcpy(&upathname, current += sizeof(code), sizeof(upathname));
	    memcpy(&upathname_size, current += sizeof(upathname), sizeof(upathname_size));
	    memcpy(&ucurrent_dir, current += sizeof(upathname_size), sizeof(ucurrent_dir));

	    char* pathname = (char*)malloc(upathname_size + 1);
	    CopyFrom(pid, pathname, upathname, upathname_size + 1);
	    result = FSChDir(pathname, ucurrent_dir);
	    free(pathname);
	    break;
	}
	case CODE_STAT:{
	    char* upathname; int upathname_size; struct Stat* ustatbuf; short ucurrent_dir;
	    memcpy(&upathname, current += sizeof(code), sizeof(upathname));
	    memcpy(&upathname_size, current += sizeof(upathname), sizeof(upathname_size));
	    memcpy(&ustatbuf, current += sizeof(upathname_size), sizeof(ustatbuf));
	    memcpy(&ucurrent_dir, current += sizeof(ustatbuf), sizeof(ucurrent_dir));

	    char* pathname = (char*)malloc(upathname_size + 1);
	    struct Stat* statbuf = (struct Stat*)malloc(sizeof(struct Stat));
	    CopyFrom(pid, pathname, upathname, upathname_size + 1);
	    result = FSStat(pathname, statbuf, ucurrent_dir);
	    CopyTo(pid, ustatbuf, statbuf, sizeof(statbuf));
	    free(pathname);
	    free(statbuf);
	    break;
	}
	case CODE_SYNC:{
	    result = FSSync();
	    break;
	}
	case CODE_SHUTDOWN:{
	    result = FSShutdown();
	    break;
	}
	default:{
	    result = ERROR;
	}

    }
    return result;
#pragma GCC diagnostic pop /*pop -Wint-to-pointer-cast ignore warning*/
}

int main(int argc, char** argv){
    
    init();
    init_free();    
    Register(FILE_SERVER);
    printf("Initialized File System\n");
    
    
    //=======================================================================================
    // test involving manually reading/writing sectors
    /*
    char block[BLOCKSIZE];
    ReadSector(1, (void*)(block));
    
    struct inode tmp_inode;
    tmp_inode.size = 30;
    memcpy(block + 1*sizeof(struct inode), &tmp_inode, sizeof(struct inode));
    WriteSector(1, block);

    struct inode_info* info2 = read_inode_from_disk(2);
    printf("size of inode %d\n", info2->inode_val->size);
    

    Halt();
    */
    //=======================================================================================
    // tests involving using read_inode/block_from_disk and syncing for persistence
    /*
    struct inode_info* info = read_inode_from_disk(2);
    info->inode_val->size = 40;
    info->dirty = 1;
    sync();
    
    struct inode_info* info2 = read_inode_from_disk(2);
    printf("size of inode %d\n", info2->inode_val->size);
    
    Halt();
    */
    //=======================================================================================
    /*
    struct block_info* info = read_block_from_disk(200);
    char* msg1 = "This should be written\n";
    memcpy(info->data, msg1, strlen(msg1+1));
    info->dirty = 1;
    sync();
    
    
    struct block_info* info2 = read_block_from_disk(200);
    printf("Msg: %s\n", (char*)info2->data);
        
    Halt();
    */
    //=======================================================================================
    
    printf("Starting Test 1\n");
    
    char* dir_name1 = "/spam1";
    char* dir_name2 = "/spam2";
    char* dir_name3 = "/spam1/foo1";
    char* dir_name4 = "foo2";
    /*                
    int result1 = FSMkDir(dir_name1, 0);
    int result2 = FSMkDir(dir_name2, 0);
    int result3 = FSMkDir(dir_name3, 0);
    int result4 = FSMkDir(dir_name4, 3);
    
    printf("result %d\n", result1);
    printf("result %d\n", result2);
    printf("result %d\n", result3);
    printf("result %d\n", result4);
    */
    sync();
    
    print_dir(1);
    print_dir(2);
    print_dir(3);
    
    printf("size of root %d\n", read_inode_from_disk(FSOpen("/", 0))->inode_val->size);
    printf("size of spam1 %d\n", read_inode_from_disk(FSOpen(dir_name1, 0))->inode_val->size);
    printf("size of spam2 %d\n", read_inode_from_disk(FSOpen(dir_name2, 0))->inode_val->size);
    printf("size of foo1 %d\n", read_inode_from_disk(FSOpen(dir_name3, 0))->inode_val->size);
    printf("size of foo2 %d\n", read_inode_from_disk(FSOpen(dir_name4, 3))->inode_val->size);

    Halt();
    
    //=======================================================================================
    // actual main method

    int pid = Fork();
    if(pid == 0){
	Exec(argv[1], argv + 1);
	printf("No init file provided. Halting machine new\n");
	Halt();
    }
    
    // stand by and simply route messages from here on out
    while(1){
	char msg[MESSAGE_SIZE];
	int pid = Receive(msg);
	if(pid == -1){
	    fprintf(stderr, "Receive() returned error\n");
	    continue;
	}
	if(pid == 0){
	    Pause();
	    fprintf(stderr, "Recieve() returned 0 to avoid deadlock\n");
	    continue;
	}
	
	int result = Redirect_Call(msg, pid);

	// clean msg
	int i;
	for(i = 0; i < MESSAGE_SIZE; i++){
	    msg[i] = '\0';
	}

	// copy in result and reply
	memcpy(msg, &result, sizeof(result));
	Reply(msg, pid);      
    }
    
    return 0;
}
