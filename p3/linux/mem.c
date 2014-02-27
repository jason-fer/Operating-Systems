#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "mem.h"

int m_error;

typedef struct __node_t {
  int size;
  void *next;
} node_t;

node_t* head = NULL;

int Mem_Init(int sizeOfRegion) {
  printf("calling: Mem_Init()!\n");

  void *ptr = mmap(NULL, sizeOfRegion, 
                   PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, 
                   -1, 0);
  if (ptr == MAP_FAILED) 
    { 
      perror("mmap"); 
      return -1;
    }

  head       = (node_t*)ptr;
  head->size = sizeOfRegion;
  head->next = NULL;

  return 0;
}


void* Mem_Alloc(int size, int style) {
  return NULL;
}

int Mem_Free(void* ptr) {
  return -1;
}

void Mem_Dump() {
  printf("dump: \n");
  node_t* tmp = head;
  while(tmp) {
    printf ("Free Size is %d \n",tmp->size);
    tmp = tmp->next;
  }
}
