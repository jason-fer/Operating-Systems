#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <limits.h>
#include <math.h>
#include "mem.h"
#include <stdint.h>

int m_error;

typedef struct __node_t node_t;

struct __node_t {
  uint32_t size;
  uint8_t free;
  node_t* next;
};


node_t* head = NULL;
int initSize;

int Mem_Init(int sizeOfRegion) {
  
  if (sizeOfRegion <= 0) {
    perror("mmap");
    m_error = E_BAD_ARGS;
    return -1;
  }

  if (head != NULL) {
    perror("mmap");
    m_error = E_PADDING_OVERWRITTEN;
    return -1;
  }

  initSize = sizeOfRegion;
  int ceilSize  = ceil(((double)sizeOfRegion/(double)getpagesize()));
  int allocSize = ceilSize*getpagesize();

  void *ptr = mmap(NULL, allocSize, 
                   PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, 
                   -1, 0);

  if (ptr == MAP_FAILED) {
    perror("mmap");
    m_error = E_CORRUPT_FREESPACE;
    return -1;
  }
  
  /* printf ("Init size is %d\n", allocSize); */
  head        = (node_t*)ptr;
  head->size  = allocSize;
  head->next  = NULL;
  head->free = 1;

  return 0;
}


void* Mem_Alloc(int size) {
  
  node_t* tmp          = head;
  node_t* bestFitNode  = NULL;
  void*   returnP      = NULL;

  int currDiff;
  int bestDiff;

  bestDiff = INT_MAX;

  while (tmp != NULL && (returnP == NULL)) {

    if ((tmp->free != 1) || (tmp->size < size) ) {
      tmp = tmp->next;
      continue;
    } 

    double pointerInc = 
      ((double)(size + sizeof(node_t)))/((double)sizeof(node_t));
    int ceilPointerInc = ceil(pointerInc);

    if ((tmp + (ceilPointerInc)) == tmp->next) {
      bestFitNode = tmp;
      break;
    }

    if (tmp->size < ceilPointerInc*sizeof(node_t)) {
      tmp = tmp->next;
      continue;
    }

    currDiff = tmp->size - size;
    if (currDiff < bestDiff) {
      bestDiff = currDiff;
      bestFitNode = tmp;
    }
    tmp = tmp->next;
  }

  if (!bestFitNode) {
    printf("Not enough memory for %d request\n", size);
    m_error = E_NO_SPACE;
    return returnP;
  }

  returnP        = bestFitNode + 1;

  double pointerInc = 
    ((double)(size + sizeof(node_t)))/((double)sizeof(node_t));
  int ceilPointerInc = ceil(pointerInc);

  int bestTmpSize = bestFitNode->size;
  bestFitNode->size = (ceilPointerInc-1)*sizeof(node_t);
  bestTmpSize -= (bestFitNode->size + sizeof(node_t));
      
  node_t* next = bestFitNode->next;
    
  //Changing best fit node
  if ((bestFitNode + ceilPointerInc) < (head + (initSize/sizeof(node_t)))) {
    bestFitNode->next = bestFitNode + ceilPointerInc;
  } else {
    bestFitNode->next = NULL;
  }
    
  bestFitNode->free = 0;
      
  if (next != bestFitNode->next) {
    bestFitNode->next->size = bestTmpSize;
    bestFitNode->next->next = next;
    bestFitNode->next->free = 1;
  }

  if (!returnP) {
    printf("Not enough memory for %d request\n", size);
    m_error = E_NO_SPACE;
  }
  
  /* Mem_Dump(); */
  return returnP;
}


int Mem_Free(void* ptr) {
  
  if (ptr == NULL) {
    return 0;
  }

  int     freed     = 0;
  int     coalesced = 0;
  node_t* tmp       = head;
  node_t* freedTmp  = NULL;

  while (tmp != NULL) {
    if ( ((node_t*)ptr) == (tmp+1) ) {
      /* printf("Deallocating %p\n", ptr); */
      tmp->free = 1;
      freed = 1;
      freedTmp = tmp;
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
        tmp->free = 1;
        tmp->next = tmp->next->next;
        coalesced = 1;
        // As the logic would just look for next 
        // we don't want to go to next before checking again
        continue; 
      } 
 
      tmp = tmp->next;
    }

    /* Mem_Dump(); */
    return 0;
  } else {
    m_error = E_BAD_POINTER;
    return -1;
  }
}

void Mem_Dump() {
  printf("dump: \n");
  node_t* tmp = head;
  while(tmp) {
    if (tmp->free == 1) {
      printf ("Free Size is %d \n",tmp->size);
    } else {
      printf ("Occupied size is %d\n", tmp->size);
    }
    tmp = tmp->next;
  }
}
