#include "yfs.h"

void Print() {

    printf("\n---- Inode hashmap -----\n");
    int i = 0;
    for(i = 0; i < INODE_CACHESIZE; i++){
	struct inode_wrap* elm = inode_hashtable[i];
	if(inode_hashtable[i] != NULL)
	    printf("Index %d - Key: %d, Pointer: %ld\n", i, elm->key, (long)elm->inode_data);
    }
    printf("\n---- Inode linked list -----\n");
    struct inode_info* temp1 = inode_front;
    while(temp1 != NULL) {
	printf("Inode Number: %d Pointer: %ld\n", temp1->inode_number, (long)temp1);
	temp1 = temp1->next;
    } 
    printf("\n");


    printf("\n---- Block hashmap -----\n");
    for(i = 0; i < BLOCK_CACHESIZE; i++){
	struct block_wrap* elm = block_hashtable[i];
	if(block_hashtable[i] != NULL)
	    printf("Index %d - Key: %d, Pointer: %ld Dirty? %d\n", i, elm->key,
		   (long)elm->block_data, elm->block_data->dirty);
    }
    printf("\n---- Block linked list -----\n");
    struct block_info* temp2 = block_front;
    while(temp2 != NULL) {
	printf("Block Number: %d Pointer: %ld Dirty? %d\n", temp2->block_number, (long)temp2,
	       temp2->dirty);
	temp2 = temp2->next;
    } 
    printf("\n");
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

    //adding for loop to initialize to null
    int i;
    for(i = 0; i < INODE_CACHESIZE; i++){
	inode_hashtable[i] = NULL;
    }
    for(i = 0; i < BLOCK_CACHESIZE; i++){
	block_hashtable[i] = NULL;
    }
}

int calculate_inode_to_block_number(int inode_number) {
    return 1 + (inode_number / (BLOCKSIZE / INODESIZE));
}

int generate_hash_code_inode(int key) {
    //Hash code generator.
    return key % INODE_CACHESIZE;
}

int generate_hash_code_block(int key) {
    //Hash code generator.
    return key % BLOCK_CACHESIZE;
}

struct block_wrap *get_block(int key) {
    //Obtains the hash code.
    int hash_index = generate_hash_code_block(key);  
    int start = hash_index;

    while(block_hashtable[hash_index] != NULL) {
	if(block_hashtable[hash_index]->key == key){
	    return block_hashtable[hash_index]; 
	}
			
	++hash_index;
		
	hash_index %= BLOCK_CACHESIZE;

	// made a full circle without finding an opening
	if(hash_index == start)
	    return NULL;
    }        
	
    return NULL;        
}

void put_block_to_hashtable(int key, struct block_info* data_input) {

    struct block_wrap *item = (struct block_wrap*) malloc(sizeof(struct block_wrap));
    item->block_data = data_input;
    item->key = key;

    int hash_index = generate_hash_code_block(key);

    while(block_hashtable[hash_index] != NULL && block_hashtable[hash_index]->key != -1) {
	++hash_index;
		
	hash_index %= BLOCK_CACHESIZE;
    }
	
    block_hashtable[hash_index] = item;
}

struct block_wrap* remove_block_from_hashtable(int block_num) {

    int hash_index = generate_hash_code_block(block_num);
    int start = hash_index;

    while(block_hashtable[hash_index] != NULL) {
	
	if(block_hashtable[hash_index]->key == block_num) {
	    struct block_wrap* temp = block_hashtable[hash_index]; 
			
	    block_hashtable[hash_index] = default_block_wrap; 

	    return temp;
	}
		
	++hash_index;
		
	hash_index %= BLOCK_CACHESIZE;
      
	// made full circule without finding opening
	if(hash_index == start)
	    return NULL;
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
	if(tmp_inode->dirty == 1) {
	    //The value is dirty.
	    int block_num_to_write = calculate_inode_to_block_number(inode_number);
	    struct block_info *tmp = read_block_from_disk(block_num_to_write);
	    memcpy((void*)(tmp->data + (inode_number - (BLOCKSIZE/INODESIZE) * (block_num_to_write - 1)) * INODESIZE), (void*)(tmp_inode->inode_val), INODESIZE);
	    tmp->dirty = 1;
	    tmp_inode->dirty = 0;
	}
	tmp_inode = tmp_inode->next;
    }
    
    while(tmp_block != NULL) {
	if(tmp_block->dirty == 1) {
	    //The block is dirty, so write it back.
	    int sig = WriteSector(tmp_block->block_number, (void*)(tmp_block->data));
	    if(sig == 0) {
		tmp_block->dirty = 0;
	    
	    }else{
		printf("ERROR: WriteSector call failed.\n");
		return ERROR;
	    }
	    //Marks the current block to not dirty.
	    tmp_block->dirty = 0;
	}
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
    int hash_index = generate_hash_code_inode(key);  
    int start = hash_index;
    while(inode_hashtable[hash_index] != NULL) {
	if(inode_hashtable[hash_index]->key == key){
	    return inode_hashtable[hash_index]; 
	}
         
	++hash_index;
      
	hash_index %= INODE_CACHESIZE;
      
	//made a full circle but couldn't find an opening
	if(hash_index == start)
	    return NULL;
    }        
   
    return NULL;        
}

void put_inode_to_hashtable(int key, struct inode_info* data_input) {

    struct inode_wrap *item = (struct inode_wrap*) malloc(sizeof(struct inode_wrap));
    item->inode_data = data_input;
    item->key = key;

    int hash_index = generate_hash_code_inode(key);

    while(inode_hashtable[hash_index] != NULL && inode_hashtable[hash_index]->key != -1) {
	++hash_index;
      
	hash_index %= INODE_CACHESIZE;
    }
   
    inode_hashtable[hash_index] = item;
}

struct inode_wrap* remove_inode_from_hashtable(int inode_num) {

    int hash_index = generate_hash_code_inode(inode_num);
    int start = hash_index;

    while(inode_hashtable[hash_index] != NULL) {
	if(inode_hashtable[hash_index]->key == inode_num) {
	    struct inode_wrap* temp = inode_hashtable[hash_index]; 
         
	    inode_hashtable[hash_index] = default_inode_wrap; 

	    return temp;
	}
      
	++hash_index;
      
	hash_index %= INODE_CACHESIZE;
      
	// made full circle without finding an opening
	if(hash_index == start)
	    return NULL;
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
