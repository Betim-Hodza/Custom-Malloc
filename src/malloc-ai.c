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
void printStatistics(void)
{
    printf("\nheap management statistics\n");
    printf("mallocs:\t%d\n", num_mallocs);
    printf("frees:\t\t%d\n", num_frees);
    printf("reuses:\t\t%d\n", num_reuses);
    printf("grows:\t\t%d\n", num_grows);
    printf("splits:\t\t%d\n", num_splits);
    printf("coalesces:\t%d\n", num_coalesces);
    printf("blocks:\t\t%d\n", num_blocks);
    printf("requested:\t%d\n", num_requested);
    printf("max heap:\t%d\n", max_heap);
}

struct _block
{
    size_t size;          /* Size of the allocated _block of memory in bytes     */
    struct _block *next;  /* Pointer to the next _block of allocated memory      */
    struct _block *prev;  /* Pointer to the previous _block of allocated memory  */
    bool free;            /* Is this _block free?                                */
    char padding[3];      /* Padding for alignment                               */
};

struct _block *heapList = NULL; /* Free list to track the _blocks available */

/*
 * \brief findFreeBlock
 *
 * \param last pointer to the linked list of free _blocks
 * \param size size of the _block needed in bytes
 *
 * \return a _block that fits the request or NULL if no free _block matches
 */
struct _block *findFreeBlock(struct _block **last, size_t size)
{
    struct _block *curr = heapList;

#if defined FIT && FIT == 0
    /* First Fit */
    while (curr && !(curr->free && curr->size >= size))
    {
        *last = curr;
        curr = curr->next;
    }
#endif

#if defined BEST && BEST == 0
    /* Best Fit */
    struct _block *best = NULL;
    while (curr)
    {
        if (curr->free && curr->size >= size)
        {
            if (!best || curr->size < best->size)
            {
                best = curr;
            }
        }
        *last = curr;
        curr = curr->next;
    }
    curr = best;
#endif

#if defined WORST && WORST == 0
    /* Worst Fit */
    struct _block *worst = NULL;
    while (curr)
    {
        if (curr->free && curr->size >= size)
        {
            if (!worst || curr->size > worst->size)
            {
                worst = curr;
            }
        }
        *last = curr;
        curr = curr->next;
    }
    curr = worst;
#endif

#if defined NEXT && NEXT == 0
    /* Next Fit */
    static struct _block *next_fit_last = NULL;
    if (!next_fit_last)
        next_fit_last = heapList;

    curr = next_fit_last;
    while (curr)
    {
        if (curr->free && curr->size >= size)
        {
            break;
        }
        *last = curr;
        curr = curr->next;
    }

    if (!curr)
    {
        curr = heapList;
        while (curr != next_fit_last)
        {
            if (curr->free && curr->size >= size)
            {
                break;
            }
            *last = curr;
            curr = curr->next;
        }
        if (curr == next_fit_last)
            curr = NULL;
    }

    next_fit_last = curr;
#endif

    return curr;
}

/*
 * \brief growHeap
 *
 * \param last tail of the free _block list
 * \param size size in bytes to request from the OS
 *
 * \return returns the newly allocated _block or NULL if failed
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

    /* Update _block metadata */
    curr->size = size;
    curr->next = NULL;
    curr->prev = last;
    curr->free = false;

    num_grows++;
    num_blocks++;
    max_heap += size + sizeof(struct _block);

    return curr;
}

/*
 * \brief malloc
 *
 * \param size size of the requested memory in bytes
 *
 * \return returns the requested memory allocation to the calling process
 * or NULL if failed
 */
void *malloc(size_t size)
{
    if (atexit_registered == 0)
    {
        atexit_registered = 1;
        atexit(printStatistics);
    }

    /* Align to multiple of 4 */
    size = ALIGN4(size);

    /* Handle 0 size */
    if (size == 0)
    {
        return NULL;
    }

    num_mallocs++;
    num_requested += size;

    /* Look for free _block */
    struct _block *last = heapList;
    struct _block *next = findFreeBlock(&last, size);

    if (next)
    {
        num_reuses++;
    }

    /* Could not find free _block, so grow heap */
    if (next == NULL)
    {
        next = growHeap(last, size);
        if (next == NULL)
        {
            return NULL;
        }
    }
    else if (next->size >= size + sizeof(struct _block) + 4)
    {
        /* Split the block */
        struct _block *new_block = (struct _block *)((char *)next + sizeof(struct _block) + size);
        new_block->size = next->size - size - sizeof(struct _block);
        new_block->free = true;
        new_block->next = next->next;
        new_block->prev = next;

        if (new_block->next)
        {
            new_block->next->prev = new_block;
        }

        next->size = size;
        next->next = new_block;

        num_splits++;
        num_blocks++;
    }

    /* Mark _block as in use */
    next->free = false;

    /* Return data address associated with _block to the user */
    return BLOCK_DATA(next);
}

/*
 * \brief free
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

    num_frees++;

    /* Mark _block as free */
    struct _block *curr = BLOCK_HEADER(ptr);
    assert(curr->free == 0);
    curr->free = true;

    /* Coalesce free _blocks */
    /* Coalesce with next block */
    if (curr->next && curr->next->free)
    {
        curr->size += sizeof(struct _block) + curr->next->size;
        curr->next = curr->next->next;
        if (curr->next)
        {
            curr->next->prev = curr;
        }
        num_coalesces++;
        num_blocks--;
    }

    /* Coalesce with previous block */
    if (curr->prev && curr->prev->free)
    {
        curr->prev->size += sizeof(struct _block) + curr->size;
        curr->prev->next = curr->next;
        if (curr->next)
        {
            curr->next->prev = curr->prev;
        }
        curr = curr->prev;
        num_coalesces++;
        num_blocks--;
    }
}

/*
 * \brief calloc
 *
 * \param nmemb number of elements
 * \param size size of each element
 *
 * \return pointer to allocated memory or NULL if failed
 */
void *calloc(size_t nmemb, size_t size)
{
    size_t total_size = nmemb * size;
    void *ptr = malloc(total_size);
    if (ptr)
    {
        memset(ptr, 0, total_size);
    }
    return ptr;
}

/*
 * \brief realloc
 *
 * \param ptr pointer to previously allocated memory
 * \param size new size in bytes
 *
 * \return pointer to reallocated memory or NULL if failed
 */
void *realloc(void *ptr, size_t size)
{
    if (ptr == NULL)
    {
        return malloc(size);
    }

    if (size == 0)
    {
        free(ptr);
        return NULL;
    }

    struct _block *curr = BLOCK_HEADER(ptr);
    size_t old_size = curr->size;
    size_t new_size = ALIGN4(size);

    num_requested += new_size;

    if (new_size <= old_size)
    {
        /* Optionally split the block */
        if (old_size - new_size >= sizeof(struct _block) + 4)
        {
            struct _block *new_block = (struct _block *)((char *)curr + sizeof(struct _block) + new_size);
            new_block->size = old_size - new_size - sizeof(struct _block);
            new_block->free = true;
            new_block->next = curr->next;
            new_block->prev = curr;

            if (new_block->next)
            {
                new_block->next->prev = new_block;
            }

            curr->size = new_size;
            curr->next = new_block;

            num_splits++;
            num_blocks++;
        }
        return ptr;
    }
    else
    {
        /* Try to coalesce with next block */
        if (curr->next && curr->next->free &&
            (curr->size + sizeof(struct _block) + curr->next->size) >= new_size)
        {
            curr->size += sizeof(struct _block) + curr->next->size;
            curr->next = curr->next->next;
            if (curr->next)
            {
                curr->next->prev = curr;
            }
            num_coalesces++;
            num_blocks--;

            if (curr->size >= new_size)
            {
                /* Optionally split the block */
                if (curr->size - new_size >= sizeof(struct _block) + 4)
                {
                    struct _block *new_block = (struct _block *)((char *)curr + sizeof(struct _block) + new_size);
                    new_block->size = curr->size - new_size - sizeof(struct _block);
                    new_block->free = true;
                    new_block->next = curr->next;
                    new_block->prev = curr;

                    if (new_block->next)
                    {
                        new_block->next->prev = new_block;
                    }

                    curr->size = new_size;
                    curr->next = new_block;

                    num_splits++;
                    num_blocks++;
                }
                return BLOCK_DATA(curr);
            }
        }

        /* Allocate new block */
        void *new_ptr = malloc(size);
        if (new_ptr)
        {
            memcpy(new_ptr, ptr, curr->size);
            free(ptr);
        }
        return new_ptr;
    }
}
