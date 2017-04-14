#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#define SIZE 50
#define BLOCKSIZE 32
#define CACHESIZE 2 // Needs to be modified to another number.
int current_blockcache_number;


//Dirty = 1 means it is dirty
//Dirty = 0 means it is not dirty

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
struct block_info* front;
struct block_info* rear;

struct block_wrap* block_head;
struct block_wrap* block_tail;

struct block_wrap* block_hashtable[SIZE]; 
struct block_wrap* default_block_wrap;
struct block_info* default_block_info;
struct block_wrap* item;

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

void print_cache() {
   int i = 0;
    
   for(i = 0; i<SIZE; i++) {
    
      if(block_hashtable[i] != NULL)
         printf(" (%d,%d)",block_hashtable[i]->key,block_hashtable[i]->key);
      else
         printf(" ~~ ");
   }
    
   printf("\n");
}


void Enqueue_block(struct block_info * x) {
   //Puts to end.
   if(front == NULL && rear == NULL){
      front = rear = x;
      front->prev = NULL;
      rear->next = NULL;
      return;
   }
   rear->next = x;
   x->prev = rear;
   rear = x;
   rear->next = NULL;
}


void Dequeue_block() {
   //Eliminate the front;
   if(front == NULL) {
      printf("Queue is Empty\n");
      return;
   }
   if(front == rear) {
      front = rear = NULL;
   }else {
      front->next->prev = NULL;
      front = front->next;
      front->prev = NULL;
   }

}

void remove_queue(struct block_info * x) {
   if(x->prev != NULL && x->next != NULL){
      x->prev->next = x->next;
      x->next->prev = x->prev;
   }else if(x->prev != NULL && x->next == NULL) {
      rear = rear->prev;
      rear->next = NULL;
   }else if(x->prev == NULL && x->next != NULL) {
      Dequeue_block();
   }
}

void Print() {
   struct block_info* temp = front;
   while(temp != NULL) {
      printf("%d\n",temp->block_number);
      temp = temp->next;
   }
   free(temp);
}


void init() {
   current_blockcache_number = 0;
   front = NULL;
   rear = NULL;
   default_block_wrap = (struct block_wrap*) malloc(sizeof(struct block_wrap));
   default_block_wrap->key = -1;

   default_block_info = (struct block_info*) malloc(sizeof(struct block_info));
   default_block_info->block_number = -1;
}

struct block_info* get_lru_block(int block_num) {
   struct block_wrap* result = get_block(block_num);
   if(result == NULL) {
      return default_block_info;
   }else{
      //Recently used.
      remove_queue(result->block_data);
      Enqueue_block(result->block_data);
      // put_to_front(result);

      return result->block_data;
   }
}

void evict(){
   //Test whether there is a key to be evicted.
   if(current_blockcache_number == CACHESIZE) {
      int to_be_removed_key = front->block_number;
      //Here should be another method sync to write inode back to the disk.





      if(front->dirty == 1) {
         WriteSector(front->block_number, (void*)(front->data));
      }
      Dequeue_block();
      remove_block_from_hashtable(to_be_removed_key);
      //Decrement the current block cache number by 1.
      current_blockcache_number--;
   }
}

void set_lru_block(int block_num, struct block_info* input_block) {
   if(get_block(block_num) == NULL) {
      // printf("Key not found\n");
      //Determines whether a key needs to be removed.
      evict();
      Enqueue_block(input_block);
      put_block_to_hashtable(block_num, input_block);
      current_blockcache_number++;
      return;
   }else{
      printf("Key already in it.\n");
      return;
   }

}


int main(int argc, char** argv){
    return 0;
}

int FSOpen(char *pathname){
    return 0;
}

int FSClose(int fd){
    return 0;
}

int FSCreate(char *pathname){
    return 0;
}

int FSRead(int fd, void *buf, int size){
    return 0;
}

int FSWrite(int fd, void *buf, int size){
    return 0;
}

int FSSeek(int fd, int offset, int whence){
    return 0;
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

int FSChDir(char *pahtname){
    return 0;
}

int FSStat(char *pathname, struct Stat*statbuf){
    return 0;
}

int FSSync(void){
    return 0;
}

int FSShutdown(void){
    return 0;
}
