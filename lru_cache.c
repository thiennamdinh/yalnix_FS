#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#define SIZE 100
// #define BLOCKSIZE 32
// #define BLOCK_CACHESIZE 2 // Needs to be modified to another number.
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
   if(current_blockcache_number == BLOCK_CACHESIZE) {
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


int main() {

   init();
   struct block_info *x1 = (struct block_info*) malloc(sizeof(struct block_info));
   x1->block_number = 1;

   struct block_info *x2 = (struct block_info*) malloc(sizeof(struct block_info));
   x2->block_number = 2;

   struct block_info *x3 = (struct block_info*) malloc(sizeof(struct block_info));
   x3->block_number = 3;

   struct block_info *x4 = (struct block_info*) malloc(sizeof(struct block_info));
   x4->block_number = 4;

   struct block_info *x5 = (struct block_info*) malloc(sizeof(struct block_info));
   x5->block_number = 5;

   set_lru_block(1, x1);
   set_lru_block(2, x2);
   // set_lru_block(3, x3);
   // set_lru_block(4, x4);
   // set_lru_block(5, x5);

   // printf("%d\n", current_blockcache_number);
   // print_cache();
   //Test case 0:
   printf("----test case 0----\n");
   printf("%d\n", get_lru_block(1)->block_number);
   set_lru_block(3, x3);
   printf("%d\n", get_lru_block(2)->block_number);
   set_lru_block(4, x4);
   printf("%d\n", get_lru_block(1)->block_number);
   printf("%d\n", get_lru_block(3)->block_number);
   printf("%d\n", get_lru_block(4)->block_number);
   // Print();


   // //Test case 1:
   // printf("----test case 1----\n");
   // Enqueue_block(x1);
   // Enqueue_block(x2);
   // Enqueue_block(x3);
   // Enqueue_block(x4);
   // Enqueue_block(x5);
   // if(front->prev == NULL && rear->next == NULL) {
   //    printf("There is nothing in front of the linked list and after the linked list.\n");
   // }
   // //Prints the list.
   // Print();
   // printf("----test case 2----\n");
   // //Result should be 1, 2, 3, 4, 5

   // //Test case 2:
   // Dequeue_block();
   // if(front->prev == NULL && rear->next == NULL) {
   //    printf("There is nothing in front of the linked list and after the linked list.\n");
   // }

   // Print();
   // //Result should be 2, 3
   // //Test case 3;
   // printf("----test case 3----\n");
   // remove_queue(front);
   // if(front->prev == NULL && rear->next == NULL) {
   //    printf("There is nothing in front of the linked list and after the linked list.\n");
   // }
   // Print();
   // //Result should be 3, 4, 5
   // printf("----test case 4----\n");
   // remove_queue(front);
   // if(front->prev == NULL && rear->next == NULL) {
   //    printf("There is nothing in front of the linked list and after the linked list.\n");
   // }
   // Print();


   // printf("----test case 5----\n");
   // remove_queue(rear);
   // if(front->prev == NULL && rear->next == NULL) {
   //    printf("There is nothing in front of the linked list and after the linked list.\n");
   // }
   // Print();

   // printf("----test case 6----\n");
   // Enqueue_block(x1);
   // Enqueue_block(x2);
   // Enqueue_block(x3);
   // remove_queue(front->next->next);
   // if(front->prev == NULL && rear->next == NULL) {
   //    printf("There is nothing in front of the linked list and after the linked list.\n");
   // }
   // Print();


}