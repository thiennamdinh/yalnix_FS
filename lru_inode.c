#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#define SIZE 100
#define BLOCKSIZE 32
#define INODE_CACHESIZE 2 // Needs to be modified to another number.
int current_inodecache_number;


//Dirty = 1 means it is dirty
//Dirty = 0 means it is not dirty

struct inode_info {
    int dirty;
    int inode_number;
    struct inode_info *next;
    struct inode_info *prev;
    struct inode *inode_val; //From the filesystem.h
    char data[BLOCKSIZE];
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
// struct inode_wrap* item;

int generate_hash_code(int key) {
   //Hash code generator.
   return key % SIZE;
}

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

void print_cache() {
   int i = 0;
   
   for(i = 0; i<SIZE; i++) {
   
      if(inode_hashtable[i] != NULL)
         printf(" (%d,%d)",inode_hashtable[i]->key,inode_hashtable[i]->key);
      else
         printf(" ~~ ");
   }
   
   printf("\n");
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
      // printf("good\n");
      inode_rear = inode_rear->prev;
      inode_rear->next = NULL;
   }else if(x->prev == NULL && x->next != NULL) {
      dequeue_inode();
   }
}

void Print() {
   struct inode_info* temp = inode_front;
   while(temp != NULL) {
      printf("%d\n",temp->inode_number);
      temp = temp->next;
   }
   free(temp);
}


void init() {
   current_inodecache_number = 0;
   inode_front = NULL;
   inode_rear = NULL;
   default_inode_wrap = (struct inode_wrap*) malloc(sizeof(struct inode_wrap));
   default_inode_wrap->key = -1;

   default_inode_info = (struct inode_info*) malloc(sizeof(struct inode_info));
   default_inode_info->inode_number = -1;
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

void evict_inode(){
   //Test whether there is a key to be evict_inodeed.
   if(current_inodecache_number == INODE_CACHESIZE) {
      int to_be_removed_key = inode_front->inode_number;
      //Here should be another method sync to write inode back to the disk.





      // if(inode_front->dirty == 1) {
      //    WriteSector(inode_front->inode_number, (void*)(inode_front->data));
      // }
      dequeue_inode();
      remove_inode_from_hashtable(to_be_removed_key);
      //Decrement the current block cache number by 1.
      current_inodecache_number--;
   }
}

void set_lru_inode(int inode_num, struct inode_info* input_inode) {
   if(get_inode(inode_num) == NULL) {
      //Determines whether a key needs to be removed.
      evict_inode();
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

void clear_lru_inode() {
   int i;
   for (i = 0;i <SIZE;i++) {
      inode_hashtable[i] = NULL;
   }
   inode_front = NULL;
   inode_rear = NULL;
   current_inodecache_number = 0;
}


int main() {

   init();
   struct inode_info *x1 = (struct inode_info*) malloc(sizeof(struct inode_info));
   x1->inode_number = 1;

   struct inode_info *x2 = (struct inode_info*) malloc(sizeof(struct inode_info));
   x2->inode_number = 2;

   struct inode_info *x3 = (struct inode_info*) malloc(sizeof(struct inode_info));
   x3->inode_number = 3;

   struct inode_info *x4 = (struct inode_info*) malloc(sizeof(struct inode_info));
   x4->inode_number = 4;

   struct inode_info *x5 = (struct inode_info*) malloc(sizeof(struct inode_info));
   x5->inode_number = 5;

   set_lru_inode(1, x1);
   set_lru_inode(2, x2);
   // set_lru_inode(3, x3);
   // set_lru_inode(4, x4);
   // set_lru_inode(5, x5);

   // printf("%d\n", current_inodecache_number);
   // print_cache();
   //Test case 0:
   // printf("----test case 0----\n");
   // printf("%d\n", get_lru_inode(1)->inode_number);
   // set_lru_inode(3, x3);
   // printf("%d\n", get_lru_inode(2)->inode_number);
   // set_lru_inode(4, x4);
   // printf("%d\n", get_lru_inode(1)->inode_number);
   // printf("%d\n", get_lru_inode(3)->inode_number);
   // printf("%d\n", get_lru_inode(4)->inode_number);
   // set_lru_inode(4, x1);
   // printf("%d\n", get_lru_inode(4)->inode_number);
   // Print();


   //Test case 1:
   printf("----test case 1----\n");
   enqueue_inode(x1);
   enqueue_inode(x2);
   enqueue_inode(x3);
   enqueue_inode(x4);
   enqueue_inode(x5);
   if(inode_front->prev == NULL && inode_rear->next == NULL) {
      printf("There is nothing in inode_front of the linked list and after the linked list.\n");
   }
   //Prints the list.
   Print();
   printf("----test case 2----\n");
   //Result should be 1, 2, 3, 4, 5

   //Test case 2:
   dequeue_inode();
   if(inode_front->prev == NULL && inode_rear->next == NULL) {
      printf("There is nothing in inode_front of the linked list and after the linked list.\n");
   }

   Print();
   //Result should be 2, 3
   //Test case 3;
   printf("----test case 3----\n");
   remove_queue_inode(inode_front);
   if(inode_front->prev == NULL && inode_rear->next == NULL) {
      printf("There is nothing in inode_front of the linked list and after the linked list.\n");
   }
   Print();
   //Result should be 3, 4, 5
   printf("----test case 4----\n");
   remove_queue_inode(inode_front);
   if(inode_front->prev == NULL && inode_rear->next == NULL) {
      printf("There is nothing in inode_front of the linked list and after the linked list.\n");
   }
   Print();


   printf("----test case 5----\n");
   remove_queue_inode(inode_rear);
   if(inode_front->prev == NULL && inode_rear->next == NULL) {
      printf("There is nothing in inode_front of the linked list and after the linked list.\n");
   }
   Print();

   printf("----test case 6----\n");
   enqueue_inode(x1);
   enqueue_inode(x2);
   enqueue_inode(x3);
   remove_queue_inode(inode_front->next->next);
   if(inode_front->prev == NULL && inode_rear->next == NULL) {
      printf("There is nothing in inode_front of the linked list and after the linked list.\n");
   }
   Print();
   clear_lru_inode();
   printf("----test case 7----\n");
   set_lru_inode(1, x1);
   set_lru_inode(2, x2);
   printf("goodm\n");
   printf("%d\n", get_lru_inode(1)->inode_number);
   printf("-------------------\n");
   set_lru_inode(1, x4);
   Print();

}