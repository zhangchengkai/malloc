/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 *
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  Blocks are never coalesced or reused.  The size of
 * a block is found at the first aligned word before the block (we need
 * it for realloc).
 *
 * This code is correct and blazingly fast, but very bad usage-wise since
 * it never frees anything.
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "mm.h"
#include "memlib.h"

/* If you want debugging output, use the following macro.  When you hand
 * in, remove the #define DEBUG line. */
#define DEBUG
#ifdef DEBUG
# define dbg_printf(...) printf(__VA_ARGS__)
#else
# define dbg_printf(...)
#endif


/* do not change the following! */
#ifdef DRIVER
/* create aliases for driver tests */
#define malloc mm_malloc
#define free mm_free
#define realloc mm_realloc
#define calloc mm_calloc
#endif /* def DRIVER */

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)


#define SIZE_T_SIZE 8

#define Nxt_node(p)  ((char**)(p))
#define PRSIZE_PTR(p) ((int*)(((char*)(p)) - 4))
#define MYSIZE_PTR(p) ((int*)(((char*)(p)) - 8))
#define PRE_BLOCK(p) ((p) - (*((int*)(((char*)(p)) - 4))) - SIZE_T_SIZE)
#define NXT_BLOCK(p) ((p) + (*((int*)(((char*)(p)) - 8))) + SIZE_T_SIZE)
#define IS_USED(p) ((*((int*)(((char*)(p)) - 8))) & 1)

/*
 * mm_init - Called when a new trace starts.
 */
#define LIST_SIZE 101
inline int Index(int x){
  return 1;
  // if(x<=8) return 0;
  // if(x<=16) return 1;
  // if(x<=32) return 2;
  // if(x<=64) return 3;
  // if(x<=256) return 4;
  // if(x<=1024) return 5;
  // if(x<=2048) return 6;
  // if(x<=8192) return 7;
  // if(x<=16384) return 8;
  // if(x<=65536) return 9;
  // return 10;
  // if(x<=160) return (x-1)/8;
  // if(x<=240) return (x-161)/16+20;
  // if(x<=400) return (x-241)/32+25;
  // if(x<=720) return (x-401)/64+30;
  // if(x<=2000) return (x-721)/128+35;
  // if(x<=4560) return (x-2001)/256+45;
  // if(x<=14800) return (x-4561)/512+55;
  // if(x<=40400) return (x-14801)/1024+75;
  // return 100;
}

char *head[LIST_SIZE];
char *tail, *Max;
int mm_init(void){
  tail = Max = NULL;
  for(int i = 0; i < LIST_SIZE; ++i) head[i] = NULL;
  return 0;
}

inline void split(char*p,size_t size){
  if(*MYSIZE_PTR(p) < size + SIZE_T_SIZE + 8) return;
  char*nxt = p + size + SIZE_T_SIZE;
  char*nnxt = NXT_BLOCK(p);
  *MYSIZE_PTR(nxt) = (*MYSIZE_PTR(p) - size - SIZE_T_SIZE);
  *PRSIZE_PTR(nxt) = size;
  if(nnxt < Max) *PRSIZE_PTR(nnxt) = *MYSIZE_PTR(nxt);
  if(tail == p) tail = nxt;
  *MYSIZE_PTR(p) = size;
  insert_block(nxt);
}
inline void *malloc(size_t size){
  size = ALIGN(size);
  size_t newsize = size + SIZE_T_SIZE;
  int xb=Index(size+1)-1;
  while(xb < LIST_SIZE && head[xb] == NULL) ++xb;
  if(xb < LIST_SIZE){
    char *p = NULL;
    for(char*q = head[xb]; q != NULL; q = *Nxt_node(q)){
      if(*MYSIZE_PTR(q) >= size){
        p = q;
        break;
      }
    }
    if(p == NULL){
      for(++xb;xb<LIST_SIZE && head[xb]==NULL;++xb);
      if(xb < LIST_SIZE) p = head[xb];
    }
    if(p != NULL){ 
      del(p);
      split(p,size);
      *MYSIZE_PTR(p) |= 1;
      return p;
    }
  }
  char*p = mem_sbrk(newsize);
  Max = p + newsize;
  p += SIZE_T_SIZE;
  *MYSIZE_PTR(p) = size | 1;
  if(tail != NULL) *PRSIZE_PTR(p) = (*MYSIZE_PTR(tail))/2*2;
  else *PRSIZE_PTR(p) = 0;
  tail = p;
  return p;
}


inline void insert_block(char*p){
  int xb = Index(*MYSIZE_PTR(p)+1)-1;
  *Nxt_node(p) = head[xb];
  head[xb] = p;
}
inline void del(char*p){
  int xb = Index(*MYSIZE_PTR(p)+1)-1;
  if(head[xb] == p){
    head[xb] = *Nxt_node(p);
    return;
  }
  for(char* las = head[xb], *now = *Nxt_node(las); now != NULL; las = now, now = *Nxt_node(now)){
    if(p == now){
      *Nxt_node(las) = *Nxt_node(now);
      return;
    }
  }
  printf("something goes wrong here in line 164\n");
}
inline void merge(char*p){
  while(*PRSIZE_PTR(p) > 0 && !IS_USED(PRE_BLOCK(p))){
    char* Pre = PRE_BLOCK(p);
    char* nxt = NXT_BLOCK(p);
    del(Pre);
    *MYSIZE_PTR(Pre) += *MYSIZE_PTR(p) + SIZE_T_SIZE;
    if(nxt < Max) *PRSIZE_PTR(nxt) = *MYSIZE_PTR(Pre);
    if(tail == p) tail = Pre;
    p = Pre;
  }
  while(NXT_BLOCK(p) < Max && !IS_USED(NXT_BLOCK(p))){
    char* nxt = NXT_BLOCK(p);
    char* nnxt = NXT_BLOCK(nxt);
    del(nxt);
    if(tail == nxt) tail = p;
    *MYSIZE_PTR(p) += *MYSIZE_PTR(nxt) + SIZE_T_SIZE;
    if(nnxt < Max) *PRSIZE_PTR(nnxt) = *MYSIZE_PTR(p);
  }
  insert_block(p);
}
void free(void *ptr){
  char* p = ptr;
  if(p == NULL) return;
  *MYSIZE_PTR(p) -= 1;
  merge(p);
}

/*
 * realloc - Change the size of the block by mallocing a new block,
 *      copying its data, and freeing the old block.  I'm too lazy
 *      to do better.
 */
void *realloc(void *oldptr, size_t size)
{
  if(size == 0) {
    free(oldptr);
    return 0;
  }
  if(oldptr == NULL) return malloc(size);
  size_t oldsize = *MYSIZE_PTR(oldptr)/2*2;
  if(size <= oldsize) return oldptr;
  size = ALIGN(size);
  if(tail == oldptr){
    mem_sbrk(size - oldsize);
    *MYSIZE_PTR(oldptr) = size | 1;
    return oldptr;
  }
  void *newptr = malloc(size);
  if(!newptr) return 0;

  /* Copy the old data. */
  if(size < oldsize) oldsize = size;
  memcpy(newptr, oldptr, oldsize);

  /* Free the old block. */
  free(oldptr);

  return newptr;
}

/*
 * calloc - Allocate the block and set it to zero.
 */
void *calloc (size_t nmemb, size_t size)
{
  size_t bytes = nmemb * size;
  void *newptr;

  newptr = malloc(bytes);
  memset(newptr, 0, bytes);

  return newptr;
}

/*
 * mm_checkheap - There are no bugs in my code, so I don't need to check,
 *      so nah!
 */
void mm_checkheap(int verbose){
	/*Get gcc to be quiet. */
	verbose = verbose;
  // printf("???\n");
}
