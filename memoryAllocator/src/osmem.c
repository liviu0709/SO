// SPDX-License-Identifier: BSD-3-Clause
#include <unistd.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <string.h>
#include "osmem.h"
#include "block_meta.h"

#define MMAP_THRESHOLD		(128 * 1024)
// ^^ Furat din test-utils.h ^^
#define PAGE_SIZE (1024 * 4)

#define ALIGNMENT 8 // must be a power of 2
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~(ALIGNMENT-1))
#define SIZE_T_SIZE (ALIGN(sizeof(struct block_meta))) // header size
// ^^ https://moss.cs.iit.edu/cs351/slides/slides-malloc.pdf

struct block_meta *head = NULL;
void *heapStart = NULL;

int debug = 0;

void removeBlock(struct block_meta *block) {
    if ( head == block )
        head = block->prev;
    if ( block->prev != NULL )
        block->prev->next = block->next;
    if ( block->next != NULL )
        block->next->prev = block->prev;
}

// Insert a block in the list sorted by address
// Head is the lowest address
void insBlock(struct block_meta *block) {
    if ( head == NULL ) {
        head = block;
        block->next = NULL;
        block->prev = NULL;
    } else {
        struct block_meta *curent = head, *ant = NULL;
        while ( curent != NULL ) {
            if ( curent > block ) {
                block->prev = curent;
                block->next = curent->next;
                if ( ant != NULL ) {
                    ant->prev = block;
                } else {
                    head = block;
                }
                curent->next = block;
                return;
            }
            ant = curent;
            curent = curent->prev;
        }
        curent = ant;
        curent->prev = block;
        block->next = curent;
        block->prev = NULL;
    }
}


// Find free block with enough space
void *lfGudBlock(size_t size) {
    struct block_meta *current = head;
    // First block with enough space works for now
    while ( current != NULL ) {
        // printf("Checking block at %p with size %d\n", current, current->size);
        if ( current->status == STATUS_FREE && current->size >= size ) {
            printf("lf: Found a block at %p with size %d for %d\n", current, current->size, size);
            return current;
        }
        current = current->prev;
    }
    return NULL;
}

void *lfFreeBlockOnlyLast() {
    struct block_meta *current = head;
    // We want the highest address ...
    // So we go all the way to the end
    while ( current->prev != NULL ) {
        current = current->prev;
    }
        if ( current->status == STATUS_FREE ) {
            return current;
        } else {
            return NULL;
        }
    return NULL;
}

void *allocMap(size_t size) {
    printf("Allocating memory with mmap\n");
    void *blk = mmap(NULL, ALIGN(size + SIZE_T_SIZE),
        PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        struct block_meta *adr = blk;
        if ( head == NULL ) {
            head = adr;
            head->next = NULL;
            head->prev = NULL;
            head->size = size;
            head->status = STATUS_MAPPED;
        } else {
            head->next = adr;
            adr->prev = head;
            adr->next = NULL;
            adr->size = size;
            adr->status = STATUS_MAPPED;
            head = adr;
        }
        return (char*)blk + SIZE_T_SIZE;
}

void *preAllocBrk(size_t size) {
    // Preallocating memory
    printf("Preallocating memory in func\n");
    void *blk;
    char *ret;
    heapStart = sbrk(MMAP_THRESHOLD);
    blk = heapStart;
    struct block_meta *block = blk;
    block->size = ALIGN(size);
    block->status = STATUS_ALLOC;
    insBlock(block);
    ret = (char*)blk + SIZE_T_SIZE;
    // Make a new block with the remaining memory
    // Check first if there is enough space for a new block
    if ( (int)(MMAP_THRESHOLD - ALIGN(size) - SIZE_T_SIZE * 2) >= 0 ) {
        printf("Creating a new block with size at first brk%d\n", MMAP_THRESHOLD - ALIGN(size) - SIZE_T_SIZE * 2);
        blk += SIZE_T_SIZE + size;
        struct block_meta *block2 = blk;
        block2->size = MMAP_THRESHOLD - size - SIZE_T_SIZE * 2;
        block2->status = STATUS_FREE;
        insBlock(block2);
    }
    return ret;
}

void coalesce() {
    struct block_meta *current = head;
    while ( current != NULL ) {
        if ( current->status == STATUS_FREE ) {
            struct block_meta *next = current->prev;
            if ( next != NULL ) {
                if ( next->status == STATUS_FREE ) {
                    current->size += next->size + SIZE_T_SIZE;
                    current->prev = next->prev;
                    if ( next->prev != NULL ) {
                        next->prev->next = current;
                    }
                    printf("Coalescing blocks at %p and %p\n", current, next);
                    // If theres more than 2 blocks to merge
                    if ( current->next != NULL )
                        current = current->next;
                }
            }
        }
        current = current->prev;
    }
}

void* splitBlock(struct block_meta *block, size_t size) {
    // Split the block for the size we need
    printf("Malloc: Found a block at %p with size %d. I need %d\n", block, block->size, size);
    if ( block->size >= size + SIZE_T_SIZE + 8 ) {
        struct block_meta *block2 = (struct block_meta*)((char*)block + SIZE_T_SIZE + ALIGN(size));
        printf("Splitting block at %p with size %d\n", block, size);
        block2->size = block->size - ALIGN(size) - SIZE_T_SIZE;
        block2->status = STATUS_FREE;
        printf("Block2 at %p with size %d\n", block2, block2->size);
        block->size = size;
        insBlock(block2);
    }
    block->status = STATUS_ALLOC;
    return (char*)block + SIZE_T_SIZE;
}

void* expandBlock(struct block_meta *block, size_t size) {
    printf("Expanding block...from %d to %d\n", block->size, ALIGN(size + SIZE_T_SIZE) - SIZE_T_SIZE);
    char *blockP = (char*)block;
    brk(blockP + ALIGN(size + SIZE_T_SIZE));
    block->size = ALIGN(size + SIZE_T_SIZE) - SIZE_T_SIZE;
    block->status = STATUS_ALLOC;
    return (char*) block + SIZE_T_SIZE;
}

void* newBlock(size_t size) {
    // Get space for another block...
    printf("getting moarrr space... fake size: %d, Real size: %d\n", ALIGN(size + SIZE_T_SIZE), ALIGN(size + SIZE_T_SIZE) - SIZE_T_SIZE);
    struct block_meta *block = sbrk(ALIGN(size + SIZE_T_SIZE));
    // lastBlock = block;
    block->size = ALIGN(size + SIZE_T_SIZE) - SIZE_T_SIZE;
    block->status = STATUS_ALLOC;
    insBlock(block);
    return (char*)block + SIZE_T_SIZE;
    // printf("Allocated space at %p or %p\n", ret, (char*)block + SIZE_T_SIZE);
}

void *os_malloc(size_t size) {
    printf("Requested size: %zu\n", size);
    printf("Aligned size: %zu\n", ALIGN(size + SIZE_T_SIZE));

    char *ret;
    if ( size == 0 )
        return NULL;
    if ( size >= MMAP_THRESHOLD )
        return allocMap(size);
    if ( heapStart == NULL )
        return preAllocBrk(size);

    // Coalescing...
    coalesce();
    // Find a free block with enough space
    struct block_meta *block = lfGudBlock(ALIGN(size));
    if ( block != NULL ) {
        return splitBlock(block, size);
        // printf("Allocated %ld bytes at %p\n", size, ret);
    } else {
        // printf("We fucked up... or not? Lets expand a block%d\n", debug);
        debug++;
        // If the last block is free and small ...
        // We can do one syscall and get more space:)
        block = lfFreeBlockOnlyLast();
        if ( block != NULL ) {
            return expandBlock(block, size);
        } else {
            return newBlock(size);
        }
    }

    // printf("allocated space at %p\n", ret);
    return ret;

}

void os_free(void *ptr)
{
	/* TODO: Implement os_free */
    printf("Free was called but idk man\n");
    if ( ptr == NULL ) {
        return;
    }
    struct block_meta *block = ptr - SIZE_T_SIZE;
    if ( block->status == STATUS_MAPPED ) {
        removeBlock(block);
        munmap(ptr - SIZE_T_SIZE, ALIGN(block->size + SIZE_T_SIZE));
    } else {
        // To be modified
        // munmap(ptr - SIZE_T_SIZE, MMAP_THRESHOLD);
        block->status = STATUS_FREE;
    }
}


void *os_calloc(size_t nmemb, size_t size)
{
	/* TODO: Implement os_calloc */
	if ( nmemb == 0 || size == 0 )
        return NULL;
    if ( PAGE_SIZE <= nmemb * size + SIZE_T_SIZE )
        return allocMap(nmemb * size);
    char *ret = os_malloc(nmemb * size);
    memset(ret, 0, size * nmemb);
    return ret;
}

int checkPrevBlock(struct block_meta* block) {
    block = block->prev;
    if ( block->status != STATUS_FREE )
        return 1;
    return 0;
}

void *os_realloc(void *ptr, size_t size)
{
	/* TODO: Implement os_realloc */
    if ( size == 0 )
        os_free(ptr);
    if ( ptr == NULL )
        return os_malloc(size);
    struct block_meta *block = ptr - SIZE_T_SIZE;
    if ( block->size < size ) {
        // block->size = size;
        // Try expand block
        int currentSpace = block->size;
        int unlucky = 0;
        while(1) {
            if ( (block->prev == NULL) || (block->prev->status != STATUS_FREE) ) {
                // Get new block
                unlucky = 1;
                break;
            } else {
                struct block_meta *next = block->prev;
                block->prev = next->prev;
                block = expandBlock(block, next->size + block->size);
                if ( block->size >= size ) {
                    break;
                }
            }
        }
        if ( unlucky ) {
            // Get new block
            void *newBlock = os_malloc(size);
            memcpy(newBlock, ptr, currentSpace);
            os_free(ptr);
            return newBlock;
        }
    }
	return NULL;
}
