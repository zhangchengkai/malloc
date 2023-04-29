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


#define SIZE_T_SIZE 24

#define Pre_PTR(p)  ((char**)(((char*)(p)) - 24))
#define Next_PTR(p) ((char**)(((char*)(p)) - 16))
#define SIZE_PTR(p) ((size_t*)(((char*)(p)) - 8))

/*
 * mm_init - Called when a new trace starts.
 */
#define LIST_SIZE 101
int Index(int x){
  if(x<=160) return (x-1)/8;
  if(x<=240) return (x-161)/16+20;
  if(x<=400) return (x-241)/32+25;
  if(x<=720) return (x-401)/64+30;
  if(x<=2000) return (x-721)/128+35;
  if(x<=4560) return (x-2001)/256+45;
  if(x<=14800) return (x-4561)/512+55;
  if(x<=40400) return (x-14801)/1024+75;
  return 100;
}// 0~120;

char *head[LIST_SIZE];
char *tail, *Max;
int mm_init(void){
  // printf("----------%d--------------\n",mem_sbrk(8));
  // printf("<%d>\n",sizeof(size_t));
  tail = Max = NULL;
  for(int i = 0; i < LIST_SIZE; ++i) head[i] = NULL;
  return 0;
}

/*
 * malloc - Allocate a block by incrementing the brk pointer.
 *      Always allocate a block whose size is a multiple of the alignment.
 */
void split(char*p,size_t size){
  if(*SIZE_PTR(p)/2 - size < SIZE_T_SIZE + 8) return;
  char*nxt = p + size + SIZE_T_SIZE;
  char*nnxt = p + *SIZE_PTR(p)/2 + SIZE_T_SIZE;
  *SIZE_PTR(nxt) = (*SIZE_PTR(p)/2 - size - SIZE_T_SIZE) * 2;
  *Pre_PTR(nxt) = p;
  *Next_PTR(nxt) = NULL;
  if(nnxt < Max) *Pre_PTR(nnxt) = nxt;
  if(tail == p) tail = nxt;
  *SIZE_PTR(p) = size * 2 + 1;
  insert_block(nxt);
}
void *malloc(size_t size){
  size = ALIGN(size);
  size_t newsize = size + SIZE_T_SIZE;
  int xb=Index(size);
  while(xb < LIST_SIZE && head[xb] == NULL){
    ++xb;
  }
  if(xb < LIST_SIZE){
    char *p = head[xb];
    // printf("size need is %d, given %d\n",size,*SIZE_PTR(p)/2);
    head[xb] = *Next_PTR(head[xb]);
    *Next_PTR(p) = NULL;
    *SIZE_PTR(p) |= 1;
    split(p,size);
    return p;
  }
  char*p = mem_sbrk(newsize);
  //dbg_printf("malloc %u => %p\n", size, p);

  if ((long)p < 0) return NULL;
  else {
    Max = p + newsize;  
    p += SIZE_T_SIZE;
    *SIZE_PTR(p) = size*2+1;
    *Pre_PTR(p) = tail;
    *Next_PTR(p) = NULL;
    tail = p;
    return p;
  }
}

/*
 * free - We don't know how to free a block.  So we ignore this call.
 *      Computers have big memories; surely it won't be a problem.
 */
void insert_block(char*p){
  int xb = Index(*SIZE_PTR(p)/2+1)-1;
  // printf("& %d &  ",*SIZE_PTR(p));
  *Next_PTR(p) = head[xb];
  head[xb] = p;
}
void del(char*p){
  int xb = Index(*SIZE_PTR(p)/2+1)-1;
  if(head[xb] == p){
    head[xb] = *Next_PTR(p);
    return;
  }
  for(char* las = head[xb], *now = *Next_PTR(las); now != NULL; las = now, now = *Next_PTR(now)){
    if(p == now){
      *Next_PTR(las) = *Next_PTR(now);
      return;
    }
  }
  // printf("something goes wrong here in line 123\n");
}
void merge(char*p){
  while(*Pre_PTR(p) != NULL && (*SIZE_PTR(*Pre_PTR(p))%2 == 0)){
    char* Pre = *Pre_PTR(p);
    if(Pre + (*SIZE_PTR(Pre))/2 + SIZE_T_SIZE != p){
      // printf("------------ your assholelike code is f**king run off! ... info: %d  %d  %d----------\n",p,Pre,*SIZE_PTR(Pre)/2);
    }
    char* nxt = p + (*SIZE_PTR(p))/2 + SIZE_T_SIZE;
    if(nxt < Max) *Pre_PTR(nxt) = Pre;
    del(Pre);
    *SIZE_PTR(Pre) += *SIZE_PTR(p) + SIZE_T_SIZE*2;
    if(tail == p) tail = Pre;
    p = Pre;
  }
  // printf("left merge end....   ");
  while(p + (*SIZE_PTR(p))/2 + SIZE_T_SIZE < Max && (*SIZE_PTR(p + (*SIZE_PTR(p))/2 + SIZE_T_SIZE) )%2 == 0){
    char* nxt = p + (*SIZE_PTR(p))/2 + SIZE_T_SIZE;
    char* nnxt = nxt + (*SIZE_PTR(nxt))/2 + SIZE_T_SIZE;
    if(nnxt < Max) *Pre_PTR(nnxt) = p;
    del(nxt);
    if(tail == nxt) tail = p;
    *SIZE_PTR(p) += *SIZE_PTR(nxt) + SIZE_T_SIZE*2;
  }
  // printf("right merge end....   ");
  insert_block(p);
}
void free(void *ptr){
  // printf("free begin here-----------------          ");
  char* p = ptr;
  if(p == NULL) return;
  *SIZE_PTR(p) &= ~0x1;
  // printf("size p is %d, merge ing.........    ",*SIZE_PTR(p)/2);
  merge(p);
  // printf("OK\n");
}

/*
 * realloc - Change the size of the block by mallocing a new block,
 *      copying its data, and freeing the old block.  I'm too lazy
 *      to do better.
 */
void *realloc(void *oldptr, size_t size)
{
  size_t oldsize;
  void *newptr;

  if(size == 0) {
    free(oldptr);
    return 0;
  }

  if(oldptr == NULL) return malloc(size);

  size = ALIGN(size);
  newptr = malloc(size);

  /* If realloc() fails the original block is left untouched  */
  if(!newptr) {
    return 0;
  }

  /* Copy the old data. */
  oldsize = *SIZE_PTR(oldptr)/2;
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
