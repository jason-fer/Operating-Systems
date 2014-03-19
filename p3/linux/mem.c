#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <limits.h>
#include <math.h>
#include "mem.h"

int m_error;

typedef struct __node_t node_t;

struct __node_t {
  int16_t size;
  int16_t free;
  node_t* next;
};


node_t* head = NULL;
int initSize;

int Mem_Init(int sizeOfRegion) {
  /* printf("calling: Mem_Init()!\n"); */
  
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

  head        = (node_t*)ptr;
  head->size  = allocSize;
  head->next  = NULL;
  head->free = 1;

  return 0;
}


void* Mem_Alloc(int size, int style) {
  /* Mem_Dump(); */
  
  node_t* tmp          = head;
  node_t* bestFitNode  = NULL;
  node_t* worstFitNode = NULL;
  void*   returnP      = NULL;

  int currDiff;
  int bestDiff;
  int worstDiff;

  switch (style) {
  
  case FIRSTFIT:
    while (tmp != NULL && (returnP == NULL)) {
      if ((tmp->free == 1) && (tmp->size > (size + sizeof(node_t)))) {
        /* printf("Remaining size: %d!!\n", tmp->size); */
        /* printf("Request size: %d!!\n", size); */
        /* printf("Size of Node %zu\n",sizeof(node_t)); */
        /* printf ("Allocating pointer %p \n", tmp); */
        returnP        = tmp + 1;
        
        node_t* next = tmp->next;

        double pointerInc     = 
          ((double)(sizeof(node_t) + size))/((double)sizeof(node_t));
        int    ceilPointerInc = ceil(pointerInc);
        int    initTmpSize    = tmp->size;
        tmp->size             = ceilPointerInc*sizeof(node_t);
        int    tmpSize        = initTmpSize - tmp->size;

        //Changing tmp node meaning current Node
        if (tmp + ceilPointerInc < head + (initSize/sizeof(node_t))) {
          tmp->next = tmp + ceilPointerInc;
        } else {
          tmp->next = NULL;
        }
        tmp->free = 0;
        
        // Setting values for next node
        if (tmp->next) {
          tmp->next->size = tmpSize;
          tmp->next->next = next;
          tmp->next->free = 1;
        }
      } else {
        tmp = tmp->next;
      }
    }
    break;

  case BESTFIT:
    bestDiff = INT_MAX;

    while (tmp != NULL && (returnP == NULL)) {

      if ((tmp->free != 1) || (tmp->size < (size + sizeof(node_t)))) {
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
    /* printf("Remaining size: %d!!\n", tmp->size); */
    /* printf("Request size: %d!!\n", size); */
    /* printf("Size of void*: %zu\n", sizeof(void*)); */
    /* printf ("Allocating pointer %p \n", tmp); */
    if (!bestFitNode) {
      break;
    }

    returnP        = bestFitNode + 1;

    double pointerInc = 
      ((double)(size + sizeof(node_t)))/((double)sizeof(node_t));
    int ceilPointerInc = ceil(pointerInc);

    int bestTmpSize = bestFitNode->size;
    bestFitNode->size = ceilPointerInc*sizeof(node_t);
    bestTmpSize -= bestFitNode->size;
      
    node_t* next = bestFitNode->next;
    
    //Changing best fit node
    if ((bestFitNode + ceilPointerInc) < (head + (initSize/sizeof(node_t)))) {
      bestFitNode->next = bestFitNode + ceilPointerInc;
    } else {
      bestFitNode->next = NULL;
    }

    bestFitNode->size = size;
    bestFitNode->free = 0;
      
    if (bestFitNode->next) {
      bestFitNode->next->size = bestTmpSize;
      bestFitNode->next->next = next;
      bestFitNode->next->free = 1;
    }

    break;

  case WORSTFIT:
    worstDiff = INT_MIN;

    while (tmp != NULL && (returnP == NULL)) {
      if ((tmp->free != 1) || (tmp->size < size)) {
        tmp = tmp->next;
        continue;
      }

      currDiff = tmp->size - size;
      if (currDiff > worstDiff) {
        worstDiff = currDiff;
        worstFitNode = tmp;
      }
      tmp = tmp->next;
    }
    /* printf("Remaining size: %d!!\n", tmp->size); */
    /* printf("Request size: %d!!\n", size); */
    /* printf("Size of void*: %zu\n", sizeof(void*)); */
    /* printf ("Allocating pointer %p \n", tmp); */
    if (!worstFitNode) {
      break;
    }

    returnP        = worstFitNode + 1;

    pointerInc = 
      ((double)(size + sizeof(node_t)))/((double)sizeof(node_t));
    ceilPointerInc = ceil(pointerInc);

    int worstTmpSize = worstFitNode->size;
    worstFitNode->size = ceilPointerInc*sizeof(node_t);
    worstTmpSize -= worstFitNode->size;
      
    next = worstFitNode->next;
    
    worstFitNode->size = size;
    worstFitNode->next = worstFitNode + ceilPointerInc;
    worstFitNode->free = 0;
      
    worstFitNode->next->size = worstTmpSize;
    worstFitNode->next->next = next;
    worstFitNode->next->free = 1;
    break;
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
    if ( ((node_t*)ptr) == (tmp+1) ) {
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
        tmp->free = 1;
        tmp->next = tmp->next->next;
        // As the logic would just look for next 
        // we don't want to go to next before checking again
        continue; 
      } 

      tmp = tmp->next;
    }
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
