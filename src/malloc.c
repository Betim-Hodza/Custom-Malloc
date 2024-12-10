#include <assert.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#define ALIGN4(s)         (((((s) - 1) >> 2) << 2) + 4)
#define BLOCK_DATA(b)     ((b) + 1)
#define BLOCK_HEADER(ptr) ((struct _block *)(ptr) - 1)

static int atexit_registered = 0;
static int num_mallocs       = 0;
static int num_frees         = 0;
static int num_reuses        = 0;
static int num_grows         = 0;
static int num_splits        = 0;
static int num_coalesces     = 0;
static int num_blocks        = 0;
static int num_requested     = 0;
static int max_heap          = 0;

/*
 *  \brief printStatistics
 *
 *  \param none
 *
 *  Prints the heap statistics upon process exit.  Registered
 *  via atexit()
 *
 *  \return none
 */
void calculate_fragmentation();

void printStatistics( void )
{
  printf("\nheap management statistics\n");
  printf("mallocs:\t%d\n", num_mallocs );
  printf("frees:\t\t%d\n", num_frees );
  printf("reuses:\t\t%d\n", num_reuses );
  printf("grows:\t\t%d\n", num_grows );
  printf("splits:\t\t%d\n", num_splits );
  printf("coalesces:\t%d\n", num_coalesces );
  printf("blocks:\t\t%d\n", num_blocks );
  printf("requested:\t%d\n", num_requested );
  printf("max heap:\t%d\n", max_heap );

  calculate_fragmentation();
}

struct _block 
{
   size_t  size;         /* Size of the allocated _block of memory in bytes     */
   struct _block *next;  /* Pointer to the next _block of allocated memory      */
   struct _block *prev;  /* Pointer to the previous _block of allocated memory  */
   bool   free;          /* Is this _block free?                                */
   char   padding[3];    /* Padding: IENTRTMzMjAgU3jMDEED                       */
};


struct _block *heapList = NULL; /* Free list to track the _blocks available */
int is_valid_block(struct _block *block);
/*
 * \brief findFreeBlock
 *
 * \param last pointer to the linked list of free _blocks
 * \param size size of the _block needed in bytes 
 *
 * \return a _block that fits the request or NULL if no free _block matches
 *
 * \TODO Implement Next Fit
 * \TODO Implement Best Fit
 * \TODO Implement Worst Fit
 */
struct _block *findFreeBlock(struct _block **last, size_t size) 
{
   struct _block *curr = heapList;

#if defined FIT && FIT == 0
   /* First fit */
   //
   // While we haven't run off the end of the linked list and
   // while the current node we point to isn't free or isn't big enough
   // then continue to iterate over the list.  This loop ends either
   // with curr pointing to NULL, meaning we've run to the end of the list
   // without finding a node or it ends pointing to a free node that has enough
   // space for the request.
   // 
   while (curr && !(curr->free && curr->size >= size)) 
   {
      *last = curr;
      curr  = curr->next;
   }
#endif

// \TODO Put your Best Fit code in this #ifdef block
#if defined BEST && BEST == 0
   /** \TODO Implement best fit here */
   // the idea is we want to search the heap list 
   // for current to be bigger than the requested size,
   // but less than the best size if possible. if theres
   // no smaller size than the best one left is the block we pick  
   struct _block *best = NULL;
   
   // loop until we find the best fit
   while (curr)
   {
      // if current is free and its size is bigger than requested
      if (curr->free && curr->size >= size)
      {
         // if best is null or if currents size is less than the best size
         if (!best || curr->size < best->size)
         {
            best = curr;
            // printf("Best: %ld\n", best->size);
         }
      }
      // else we set the last block to be current
      *last = curr;
      // and set current to be the next one
      curr = curr->next;
   }

   // choose the best block
   curr = best;
#endif

// \TODO Put your Worst Fit code in this #ifdef block
#if defined WORST && WORST == 0
   /** \TODO Implement worst fit here */
   // Super similar to best fit, except for we want to find the
   // largest space thats left over instead of the smallest.

   struct _block *worst = NULL;
   
   // loop until we find the best fit
   while (curr)
   {
      // if current is free and its size is bigger than requested
      if (curr->free && curr->size >= size)
      {
         // *this is the biggest change* 
         // if worst is null or 
         // if currents size is greater than the worst size, 
         // we want to update wosrt blk
         if (!worst || curr->size > worst->size)
         {
            worst = curr;
            // printf("worst: %ld\n", worst->size);
         }
      }
      // else we set the last block to be current
      *last = curr;
      // and set current to be the next one
      curr = curr->next;
   }

   // choose the best block
   curr = worst;
#endif

// \TODO Put your Next Fit code in this #ifdef block
#if defined NEXT && NEXT == 0
   /** \TODO Implement next fit here */
   
   // we need to cont searching where we left off, and wrap around to the start if needed
   // last block to fit in our search
   static struct _block *last_fit = NULL;
   // if this is our 1st time running or at end of heap, start at beginning.

   if (heapList == NULL)
   {
      curr = NULL;
   }
   else
   {
      if(!last_fit || !is_valid_block(last_fit))
      {
         last_fit = heapList;
      }

      // similar to FF here, but we start from where we left off
      curr = last_fit;
      *last = NULL;
      
      // Use a do...while loop to ensure we check all blocks, including last_fit
      do
      {
         if (curr->free && curr->size >= size)
         {
            // Found a suitable block
            break;
         }

         *last = curr;

         // wrap around
         curr = curr->next ? curr->next : heapList;
      } while (curr != last_fit);

      // After looping, check if we found a suitable block
      if (!(curr->free && curr->size >= size))
      {
         // No suitable block found
         curr = NULL;
      }
   }
   
   // update our last fit to where we currently are
   if (curr)
   {
      last_fit = curr;
   }

#endif

   return curr;
}

int is_valid_block(struct _block *block)
{
   struct _block *curr = heapList;
   while (curr)
   {
      if (curr == block)
      {
         // valid block
         return 1;
      }
      curr = curr->next;
   }
   // blk not found in list
   return 0;
}

/*
 * \brief growheap
 *
 * Given a requested size of memory, use sbrk() to dynamically 
 * increase the data segment of the calling process.  Updates
 * the free list with the newly allocated memory.
 *
 * \param last tail of the free _block list
 * \param size size in bytes to request from the OS
 *
 * \return returns the newly allocated _block of NULL if failed
 */
struct _block *growHeap(struct _block *last, size_t size) 
{
   /* Request more space from OS */
   struct _block *curr = (struct _block *)sbrk(0);
   struct _block *prev = (struct _block *)sbrk(sizeof(struct _block) + size);

   assert(curr == prev);

   /* OS allocation failed */
   if (curr == (struct _block *)-1) 
   {
      return NULL;
   }

   /* Update heapList if not set */
   if (heapList == NULL) 
   {
      heapList = curr;
   }

   /* Attach new _block to previous _block */
   if (last) 
   {
      last->next = curr;
   }

   /* Update _block metadata:
      Set the size of the new block and initialize the new block to "free".
      Set its next pointer to NULL since it's now the tail of the linked list.
      Set its prev point to Last to keep a doubly linked list
   */
   curr->size = size;
   curr->next = NULL;
   curr->prev = last;
   curr->free = false;

   // our number of blocks and grows increase by 1
   num_grows++;
   num_blocks++;
   // increase our max heaps size
   max_heap += size + sizeof(struct _block);

   return curr;
}

/*
 * \brief malloc
 *
 * finds a free _block of heap memory for the calling process.
 * if there is no free _block that satisfies the request then grows the 
 * heap and returns a new _block
 *
 * \param size size of the requested memory in bytes
 *
 * \return returns the requested memory allocation to the calling process 
 * or NULL if failed
 */
void *malloc(size_t size) 
{

   if( atexit_registered == 0 )
   {
      atexit_registered = 1;
      atexit( printStatistics );
   }

   /* Align to multiple of 4 */
   size = ALIGN4(size);

   /* Handle 0 size */
   if (size == 0) 
   {
      return NULL;
   }

   // increment the mallocs called and the size user requests
   num_mallocs++;
   num_requested += size;

   /* Look for free _block.  If a free block isn't found then we need to grow our heap. */

   struct _block *last = heapList;
   struct _block *next = findFreeBlock(&last, size);

   // if we did find a prev used block: Great! increment num_resuses
   if (next != NULL)
   {
      num_reuses++;
   }
   /* TODO: If the block found by findFreeBlock is larger than we need then:
            If the leftover space in the new block is greater than the sizeof(_block)+4 then
            split the block.
            If the leftover space in the new block is less than the sizeof(_block)+4 then
            don't split the block.
   */
   // essentially checks both conditions in 1 (pretty clever way to do it chatgpt)
   if (next != NULL && next->size >= size + sizeof(struct _block) + 4)
   {
      // splitting the block

      // Creating a new block w/ our original one. using typcasting we can do byte ptr arithmitic
      struct _block *newBlock = (struct _block *)((char *) next + sizeof(struct _block) + size);
      // we set our size for the new block to the ogblk - size (bytes) - size of metadata
      newBlock->size = next->size - size - sizeof(struct _block);
      // our new block is gonna be the one w/ the free data
      newBlock->free = true;

      // updating our linked list:
      // we set the ptr in front and behind
      // next is our og block
      newBlock->next = next->next;
      newBlock->prev = next;

      // double check that the prev point points back to the newblock
      if (newBlock->next)
      {
         // we set
         newBlock->next->prev = newBlock;
      }

      // increment our counts
      num_splits++;
      num_blocks++;
   }

 
   // split or not to split

   /* Could not find free _block, so grow heap */
   if (next == NULL) 
   {
      next = growHeap(last, size);
   }

   /* Could not find free _block or grow heap, so just return NULL */
   if (next == NULL) 
   {
      return NULL;
   }
  

   
   /* Mark _block as in use */
   next->free = false;

   /* Return data address associated with _block to the user */
   return BLOCK_DATA(next);
}

/*
 * \brief free
 *
 * frees the memory _block pointed to by pointer. if the _block is adjacent
 * to another _block then coalesces (combines) them
 *
 * \param ptr the heap memory to free
 *
 * \return none
 */
void free(void *ptr) 
{
   if (ptr == NULL) 
   {
      return;
   }

   // we are freeing stuff so increment it 
   num_frees++;

   /* Make _block as free */
   struct _block *curr = BLOCK_HEADER(ptr);
   assert(curr->free == 0);
   curr->free = true;

   /* TODO: Coalesce free _blocks.  If the next block or previous block 
            are free then combine them with this block being freed.
   */
   // visually thinking about this:
   // [prev] ? free
   // [curr]
   // [next] ? free
   // merge with whichever first is true

  // since free is a bool we can check its condition here
  // as well as checking the ptr itself

//   printf("blk has been set free, testing coalesce next\n");
  
 
  if (curr && curr->prev && curr->prev->free)
  {
   // printf("coalesce curr->prev\n");
   // coalesce curr and prev blocks 

   // combine our previous blocks size w/ our current ones size and header
   curr->prev->size += sizeof(struct _block) + curr->size;
   // set the prev next ptr to be the current ones
   curr->prev->next = curr->next;
   // if curr->next ptr is null
   if (curr->next && curr->next->prev)
   {
      // set its next->prev to be the current ones prev
      curr->next->prev = curr->prev;
   }
   // mv or ptr back
   curr = curr->prev;

   // update counts
   num_coalesces++;
   num_blocks--;
  }
  
  if (curr && curr->next && curr->next->free)
  {
   // printf("coalesce curr->next\n");
   // coalesce curr and next blocks 

   // combine our current blk size w/ our next one
   curr->size += sizeof(struct _block) + curr->next->size;
   // set the next ptr ahead
   curr->next = curr->next->next;


   if (curr->next && curr->next->prev)
   {
      // set the prev ptr back to where it was
      curr->next->prev = curr;
   }
   num_coalesces++;
   num_blocks--;

  } 
}

// allocates memory and initializes all bytes to 0
void *calloc( size_t nmemb, size_t size )
{
   // \TODO Implement calloc

   // calculate total size of the whole block including metadata
   size_t totalSize = nmemb * size;

   // use malloc to allocate memory
   void *ptr = malloc(totalSize);
   // if the allocation is successful, use memset to set all bytes to 0
   if (ptr)
   {
      memset(ptr, 0, totalSize);
   }

   return ptr;
}

// resize a previously allocated block of memory
void *realloc( void *ptr, size_t size )
{
   // \TODO Implement realloc

   // if we have an empty pointer,
   if (!ptr)
   {
      // allocate it w/ malloc
      ptr = malloc(size);
      return ptr;
   }
   else if (size == 0)
   {
      // we want to free our block of memory
      free(ptr);
      return NULL;
   }
   else 
   {
      // get the block header from the ptr
      struct _block *curr = BLOCK_HEADER(ptr);

      // get the old size and the new size
      size_t oldSize = curr->size;
      size_t newSize = ALIGN4(size);

      // increment our number of bytes requested for our counter
      num_requested += newSize;

      // if the new one is smaller than the old one
      if (newSize <= oldSize)
      {
         // split the blocks if possible
         if (oldSize - newSize >= sizeof(struct _block) + 4)
         {
            struct _block *newBlock = (struct _block *)((char *)curr + sizeof(struct _block) + newSize);
            newBlock->size = oldSize - newSize - sizeof(struct _block);
            newBlock->free = true;
            newBlock->next = curr->next;
            newBlock->prev = curr;

            if (newBlock->next)
            {
               newBlock->next->prev = newBlock;
            }

            curr->size = newSize;
            curr->next = newBlock;

            num_splits++;
            num_blocks++;
         }
         return ptr;
      }
      else // if its not smaller
      {
         // check to coalesce w/ the next blk
         if (curr->next && curr->next->free)
         {
            // printf("coalesce curr->next\n");
            if ( (curr->size + sizeof(struct _block) + curr->next->size) >= newSize )
            {
               // coalesce curr and next blocks 

               // combine our current blk size w/ our next one
               curr->size += sizeof(struct _block) + curr->next->size;
               // set the next ptr ahead
               curr->next = curr->next->next;


               if (curr->next)
               {
                  // set the prev ptr back to where it was
                  curr->next->prev = curr;
               }
               num_coalesces++;
               num_blocks--;
            }

            // check to split the blocks
            if (curr->size >= newSize)
            {
               // split our blocks
               if (curr->size - newSize >= sizeof(struct _block) + 4)
               {
                  struct _block *newBlock = (struct _block *)((char *)curr + sizeof(struct _block) + newSize);
                  newBlock->size = curr->size - newSize - sizeof(struct _block);
                  newBlock->free = true;
                  newBlock->next = curr->next;
                  newBlock->prev = curr;

                  if (newBlock->next)
                  {
                     newBlock->next->prev = newBlock;
                  }

                  curr->size = newSize;
                  curr->next = newBlock;

                  num_splits++;
                  num_blocks++;
               }
               // want to return the data of the blk
               return BLOCK_DATA(curr);
            }
            

         }

         // allocate new block
         void *newPtr = malloc(size);
         if (newPtr)
         {
            // if allocation is good, copy contents from old to new blk
            memcpy(newPtr, ptr, curr->size);
            // dont forget to free the old ptr
            free(ptr);
         } 
         return newPtr;
      }
   }
}



/* vim: IENTRTMzMjAgU3ByaW5nIDIwM001= ----------------------------------------*/
/* vim: set expandtab sts=3 sw=3 ts=6 ft=cpp: --------------------------------*/


void calculate_fragmentation() 
{
   struct _block *curr = heapList;
   size_t total_free_memory = 0;
   size_t largest_free_block = 0;
   int free_blocks = 0;

   while (curr) 
   {
      if (curr->free) 
      {
         total_free_memory += curr->size;
         if (curr->size > largest_free_block) 
         {
            largest_free_block = curr->size;
         }
         free_blocks++;
      }
      curr = curr->next;
   }

   double fragmentation_ratio = 0.0;
   if (total_free_memory > 0) 
   {
      fragmentation_ratio = 1.0 - ((double)largest_free_block / total_free_memory);
   }

   printf("Total Free Memory: %zu bytes\n", total_free_memory);
   printf("Largest Free Block: %zu bytes\n", largest_free_block);
   printf("Free Blocks: %d\n", free_blocks);
   printf("Fragmentation Ratio: %.6f\n", fragmentation_ratio);
}