#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "mem.h"

int m_error;

typedef struct __node_t node_t;

struct __node_t {
  int size;
  node_t* next;
  int free;
};


node_t* head = NULL;

int Mem_Init(int sizeOfRegion) {
  /* printf("calling: Mem_Init()!\n"); */

  void *ptr = mmap(NULL, sizeOfRegion, 
                   PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, 
                   -1, 0);

  if (ptr == MAP_FAILED) {
    perror("mmap");
    m_error = E_CORRUPT_FREESPACE;
    return -1;
  }

  head        = (node_t*)ptr;
  head->size  = sizeOfRegion;
  head->next  = NULL;
  head->free = 1;

  return 0;
}


void* Mem_Alloc(int size, int style) {
  
  node_t* tmp = head;
  void* returnP = NULL;

  switch (style) {
  
  case FIRSTFIT:
    while (tmp != NULL && (returnP == NULL)) {
      if ((tmp->free == 1) && (tmp->size > size)) {
        /* printf("Remaining size: %d!!\n", tmp->size); */
        /* printf("Request size: %d!!\n", size); */
        /* printf("Size of void*: %zu\n", sizeof(void*)); */
        /* printf ("Allocating pointer %p \n", tmp); */
        returnP        = tmp;
        
        int tmpSize = tmp->size -size;
        
        node_t* next = tmp->next;

        tmp->size = size;
        tmp->next = tmp + (size/sizeof(node_t)) + 1;
        tmp->free = 0;
        
        tmp->next->size = tmpSize;
        tmp->next->next = next;
        tmp->next->free = 1;
      } else {
        tmp = tmp->next;
      }
    }
    break;

  case BESTFIT:
  case WORSTFIT:
    printf("Not defined yet\n");
    return NULL;
  }
  
  if (!returnP) {
    printf("Not enough memory for %d request\n", size);
    m_error = E_NO_SPACE;
  }
  
  return returnP;
}


int Mem_Free(void* ptr) {

  if (ptr == NULL) {
    return 0;
  }

  int freed = 0;
  node_t* tmp = head;

  while (tmp != NULL) {
    if ( ((node_t*)ptr) == (tmp) ) {
      /* printf("Deallocating %p\n", ptr); */
      tmp->free = 1;
      freed = 1;
      break;
    } else {
      tmp = tmp->next;
    }
  }

  if (freed) {
    tmp = head;

    while (tmp != NULL && (tmp->next != NULL)) {
      if ( (tmp->free == 1) &&
           (tmp->next->free == 1) ) {
        tmp->size += tmp->next->size + sizeof(node_t);
        tmp->next = tmp->next->next;
      }
      tmp = tmp->next;
    }
    return 0;
  }

  return -1;
}

void Mem_Dump() {
  printf("dump: \n");
  node_t* tmp = head;
  while(tmp) {
    if (tmp->free == 1) {
      printf ("Free Size is %d \n",tmp->size);
      tmp = tmp->next;
    }
  }
}
