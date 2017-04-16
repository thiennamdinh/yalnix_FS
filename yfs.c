#include "yfs.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <comp421/iolib.h>
#include <comp421/filesystem.h>
#include <comp421/yalnix.h>
#include <comp421/hardware.h>

#define SIZE 100
#define BLOCKSIZE 32
#define CACHESIZE 2 // Needs to be modified to another number.



//Dirty = 1 means it is dirty
//Dirty = 0 means it is not dirty

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
   }else if(x->prev != NULL && x->next == NULL) {
      block_rear = block_rear->prev;
      block_rear->next = NULL;
   }else if(x->prev == NULL && x->next != NULL) {
      dequeue_block();
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
   }else if(x->prev != NULL && x->next == NULL) {
      inode_rear = inode_rear->prev;
      inode_rear->next = NULL;
   }else if(x->prev == NULL && x->next != NULL) {
      dequeue_inode();
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

block_info* read_block_from_disk(int block_num) {
	block_info* result = get_lru_block(block_num);
	if(result == NULL) {
		//Reads from the disk.
		result = (block_info*)malloc(sizeof(block_info));
		ReadSector(block_num, (void*)(result->data));
		//Sets the dirty to not dirty
		result->dirty = 0;
		set_lru_block(block_num, result);
		//To obtain the block_info.
		return get_lru_block(block_num);

	}else{
		return result;
	}
}

// node_info* read_inode_from_disk(int inode_num) {
// 	inode_info* result = get_lru_inode(inode_num);
// 	if(result == NULL) {
// 		int block_num = calculate_inode_to_block_number(inode_num);
// 		block_info* tmp = get_lru_block(block_num);
// 		if(tmp == NULL) {
// 			tmp = read_block_from_disk(block_num);

// 		}else{
// 			//The block is in the cache.

// 		}
// 	}else{
// 		return result;
// 	}
// }


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
	      	block_info *tmp = read_block_from_disk(block_num_to_write);
	      	int index = inode_num - (BLOCKSIZE/INODESIZE) * (block_num_to_write - 1);
	      	memcpy((void*)(tmp->data + index * INODESIZE), (void*)(&inode_front->inode_val), INODESIZE);

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

int sync() {
	//Write everything back to the disk. Syncs everything.
	inode_info* tmp_inode = inode_front;
	block_info* tmp_block = block_front;

	while(tmp_inode != NULL) {
		int inode_number = tmp_inode->inode_number;
		if(tmp_inode->dirty == 1) {
			//The value is dirty.
	      	int block_num_to_write = calculate_inode_to_block_number(to_be_removed_key);
	      	block_info *tmp = read_block_from_disk(block_num_to_write);
	      	int index = inode_num - (BLOCKSIZE/INODESIZE) * (block_num_to_write - 1);
	      	memcpy((void*)(tmp->data + index * INODESIZE), (void*)(&inode_front->inode_val), INODESIZE);
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
				printf("An error is generated when doing WriteSector.\n");
				return -1;
			}
			//Marks the current block to not dirty.
			tmp_block->dirty = 0;
		}
		tmp_block = tmp_block->next;
	}
	return 0;
}

int calculate_inode_to_block_number(int inode_number) {
	return 1 + inode_number / (BLOCKSIZE / INODESIZE);
}




int FSOpen(char *pathname){
    return 0;
}

int FSCreate(char *pathname){
    return 0;
}

int FSRead(void *buf, int size, short inode, int position){
    return 0;
}

int FSWrite(void *buf, int size, short inode, int position){
    return 0;
}

int FSSeek(short inode){
    return 500;
}

int FSLink(char *oldname, char *newname){
    return 0;
}

int FSUnlink(char *pathname){
    return 0;
}

int FSSymLink(char *oldname, char *newname){
    return 0;
}

int FSReadLink(char *pathname, char *buf, int len){
    return 0;
}

int FSMkDir(char *pathname){
    return 0;
}

int FSRmDir(char *pathname){
    return 0;
}

int FSChDir(char *pathname){
    return 0;
}

int FSStat(char *pathname, struct Stat* statbuf){
    return 0;
}

int FSSync(void){
    return 0;
}

int FSShutdown(void){
    return 0;
}

int Redirect_Call(char* msg, int pid){
#pragma GCC diagnostic push /*ignore unavoidable gcc warning caused by unconventional casting */
#pragma GCC diagnostic ignored "-Wint-to-pointer-cast"
    
    char* current = msg;
    uint8_t code = (uint8_t)(msg[0]);
    int result;
        
    switch(code){
	case CODE_OPEN:{
	    char* upathname; int upathname_size;
	    memcpy(&upathname, current += sizeof(code), sizeof(upathname));
	    memcpy(&upathname_size, current += sizeof(upathname), sizeof(upathname_size));

	    char* pathname = (char*)malloc(upathname_size + 1);
	    CopyFrom(pid, pathname, upathname, upathname_size + 1);
	    result = FSOpen(pathname);
	    free(pathname);
	    break;
	}
	case CODE_CREATE:{
	    char* upathname; int upathname_size;
	    memcpy(&upathname, current += sizeof(code), sizeof(upathname));
	    memcpy(&upathname_size, current += sizeof(upathname), sizeof(upathname_size));

	    char* pathname = (char*)malloc(upathname_size + 1);
	    CopyFrom(pid, pathname, upathname, upathname_size + 1);
	    result = FSCreate(pathname);
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
	    memcpy(&uoldname, current += sizeof(code), sizeof(uoldname));
	    memcpy(&uoldname_size, current += sizeof(uoldname), sizeof(uoldname_size));
	    memcpy(&unewname, current += sizeof(unewname_size), sizeof(unewname));
	    memcpy(&unewname_size, current += sizeof(unewname), sizeof(unewname_size));

	    char* oldname = malloc(uoldname_size + 1);
	    char* newname = malloc(unewname_size + 1);
	    CopyFrom(pid, oldname, uoldname, uoldname_size + 1);
	    CopyFrom(pid, newname, unewname, unewname_size + 1);
	    result = FSLink(oldname, newname);
	    break;
	}
	case CODE_UNLINK:{
	    char* upathname; int upathname_size;
	    memcpy(&upathname, current += sizeof(code), sizeof(upathname));
	    memcpy(&upathname_size, current += sizeof(upathname), sizeof(upathname_size));

	    char* pathname = (char*)malloc(upathname_size + 1);
	    CopyFrom(pid, pathname, upathname, upathname_size + 1);
	    result = FSUnlink(pathname);
	    free(pathname);
	    break;
	}
	case CODE_SYMLINK:{
	    char* uoldname; int uoldname_size; char* unewname; int unewname_size;
	    memcpy(&uoldname, current += sizeof(code), sizeof(uoldname));
	    memcpy(&uoldname_size, current += sizeof(uoldname), sizeof(uoldname_size));
	    memcpy(&unewname, current += sizeof(unewname_size), sizeof(unewname));
	    memcpy(&unewname_size, current += sizeof(unewname), sizeof(unewname_size));

	    char* oldname = malloc(uoldname_size + 1);
	    char* newname = malloc(unewname_size + 1);
	    CopyFrom(pid, oldname, uoldname, uoldname_size + 1);
	    CopyFrom(pid, newname, unewname, unewname_size + 1);
	    result = FSSymLink(oldname, newname);
	    break;
	}
	case CODE_READLINK:{
	    char* upathname; int upathname_size; char* ubuf; int ulen;
	    memcpy(&upathname, current += sizeof(code), sizeof(upathname));
	    memcpy(&upathname_size, current += sizeof(upathname), sizeof(upathname_size));
	    memcpy(&ubuf, current += sizeof(upathname_size), sizeof(ubuf));
	    memcpy(&ulen, current += sizeof(ubuf), sizeof(ulen));
	    
	    char* pathname = malloc(upathname_size + 1);
	    char* buf = malloc(ulen + 1);
	    CopyFrom(pid, pathname, upathname, upathname_size + 1);
	    result = FSReadLink(pathname, buf, ulen);
	    CopyTo(pid, ubuf, buf, ulen + 1);
	    free(pathname);
	    free(buf);
	    break;
	}
	case CODE_MKDIR:{
	    char* upathname; int upathname_size;
	    memcpy(&upathname, current += sizeof(code), sizeof(upathname));
	    memcpy(&upathname_size, current += sizeof(upathname), sizeof(upathname_size));

	    char* pathname = (char*)malloc(upathname_size + 1);
	    CopyFrom(pid, pathname, upathname, upathname_size + 1);
	    result = FSMkDir(pathname);
	    free(pathname);
	    break;
	}
	case CODE_RMDIR:{
	    char* upathname; int upathname_size;
	    memcpy(&upathname, current += sizeof(code), sizeof(upathname));
	    memcpy(&upathname_size, current += sizeof(upathname), sizeof(upathname_size));

	    char* pathname = (char*)malloc(upathname_size + 1);
	    CopyFrom(pid, pathname, upathname, upathname_size + 1);
	    result = FSRmDir(pathname);
	    free(pathname);
	    break;
	}
	case CODE_CHDIR:{
	    char* upathname; int upathname_size;
	    memcpy(&upathname, current += sizeof(code), sizeof(upathname));
	    memcpy(&upathname_size, current += sizeof(upathname), sizeof(upathname_size));

	    char* pathname = (char*)malloc(upathname_size + 1);
	    CopyFrom(pid, pathname, upathname, upathname_size + 1);
	    result = FSChDir(pathname);
	    free(pathname);
	    break;
	}
	case CODE_STAT:{
	    char* upathname; int upathname_size; struct Stat* ustatbuf;
	    memcpy(&upathname, current += sizeof(code), sizeof(upathname));
	    memcpy(&upathname_size, current += sizeof(upathname), sizeof(upathname_size));
	    memcpy(&ustatbuf, current += sizeof(upathname_size), sizeof(ustatbuf));

	    char* pathname = (char*)malloc(upathname_size + 1);
	    struct Stat* statbuf = (struct Stat*)malloc(sizeof(struct Stat));
	    CopyFrom(pid, pathname, upathname, upathname_size + 1);
	    result = FSStat(pathname, statbuf);
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

    int pid = Fork();
    if(pid == 0){
	Exec(argv[1], argv + 1);
    }

    Register(FILE_SERVER);

    // stand by and simply route messages from here on out
    while(1){
	char msg[MESSAGE_SIZE];
	int pid = Receive(msg);
	if(pid == -1){
	    fprintf(stderr, "Receive() returned error\n");
	    continue;
	}
	if(pid == 0){
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
