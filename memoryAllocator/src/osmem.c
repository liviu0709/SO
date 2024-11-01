// SPDX-License-Identifier: BSD-3-Clause
#include <unistd.h>
#include <stdlib.h>
#include <sys/mman.h>
#include "osmem.h"
#include "block_meta.h"

#define MMAP_THRESHOLD		(128 * 1024)
// ^^ Furat din test-utils.h ^^

#define ALIGNMENT 8 // must be a power of 2
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~(ALIGNMENT-1))
#define SIZE_T_SIZE (ALIGN(sizeof(struct block_meta))) // header size
// ^^ https://moss.cs.iit.edu/cs351/slides/slides-malloc.pdf

struct block_meta *head = NULL;
void *heapStart = NULL;

int debug = 0;

void *lfGudBlock(size_t size) {
    struct block_meta *current = head;
    while ( current != NULL ) {
        printf("Checking block at %p with size %d\n", current, current->size);
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
    while ( current != NULL ) {
        if ( current->status == STATUS_FREE ) {
            return current;
        } else {
            return NULL;
        }
        current = current->prev;
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
        return blk + SIZE_T_SIZE;
}

void *preAllocBrk(size_t size) {
    // Preallocating memory
    printf("Preallocating memory\n");
    void *blk;
    char *ret;
    heapStart = sbrk(MMAP_THRESHOLD);
    blk = heapStart;
    struct block_meta *block = blk;
    block->size = ALIGN(size);
    block->status = STATUS_ALLOC;
    if ( head == NULL ) {
        head = block;
        head->next = NULL;
        head->prev = NULL;
        // head->status = STATUS_ALLOC;
        // head->size = size;
    } else {
        head->next = block;
        block->prev = head;
        block->next = NULL;
        // block->status = STATUS_ALLOC;
        // block->size = size;
        head = block;
    }
    ret = (char*)blk + SIZE_T_SIZE;
    // Make a new block with the remaining memory
    // Check first if there is enough space for a new block
    if ( (int)(MMAP_THRESHOLD - ALIGN(size) - SIZE_T_SIZE * 2) >= 0 ) {
        printf("Creating a new block with size at first brk%d\n", MMAP_THRESHOLD - ALIGN(size) - SIZE_T_SIZE * 2);
        blk += SIZE_T_SIZE + size;
        struct block_meta *block2 = blk;
        block2->size = MMAP_THRESHOLD - size - SIZE_T_SIZE * 2;
        block2->status = STATUS_FREE;
        block2->prev = head;
        block2->next = NULL;
        head->next = block2;
        head = block2;
    }
    return ret;
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
    // Find a free block with enough space
    struct block_meta *block = lfGudBlock(ALIGN(size));
    if ( block != NULL ) {
        // Split the block for the size we need
        printf("Malloc: Found a block at %p with size %d. I need %d\n", block, block->size, size);
        if ( block->size >= size + SIZE_T_SIZE + 8 ) {
            struct block_meta *block2 = (char*)block + SIZE_T_SIZE + ALIGN(size);
            printf("Splitting block at %p with size %d\n", block, size);
            block2->size = block->size - ALIGN(size) - SIZE_T_SIZE;
            block2->status = STATUS_FREE;
            block2->prev = block;
            block2->next = block->next;
            printf("Block2 at %p with size %d\n", block2, block2->size);
            block->size = size;
            //if ( block->next == NULL ) {
                head = block2;
                printf("Head is now %p\n", head);
            //}
            block->next = block2;
        }
        block->status = STATUS_ALLOC;
        ret = (char*)block;
        ret += SIZE_T_SIZE;
        return ret;
        // printf("Allocated %ld bytes at %p\n", size, ret);
    } else {
        // printf("We fucked up... or not? Lets expand a block%d\n", debug);
        debug++;
        // If the last block is free and small ...
        // We can do one syscall and get more space:)
        block = lfFreeBlockOnlyLast();
        if ( block != NULL ) {
            // Get moar space .. tbh no ideea how much
            printf("Expanding block...from %d to %d\n", block->size, ALIGN(size + SIZE_T_SIZE) - SIZE_T_SIZE);
            char *blockP = (char*)block;
            brk(blockP + ALIGN(size + SIZE_T_SIZE));
            block->size = ALIGN(size + SIZE_T_SIZE) - SIZE_T_SIZE;
            block->status = STATUS_ALLOC;
            ret = (char*) block + SIZE_T_SIZE;
            return ret;
        } else {
            // Get space for another block...
            printf("getting moarrr space... fake size: %d, Real size: %d\n", ALIGN(size + SIZE_T_SIZE), ALIGN(size + SIZE_T_SIZE) - SIZE_T_SIZE);
            struct block_meta *block = sbrk(ALIGN(size + SIZE_T_SIZE));
            // struct block_meta *block = sbrk(size);
            block->prev = head;
            block->size = ALIGN(size + SIZE_T_SIZE) - SIZE_T_SIZE;
            block->status = STATUS_ALLOC;
            head->next = block;
            ret = (char*)block + SIZE_T_SIZE;
            // Pointer aritmetic sucks...
            // ret += SIZE_T_SIZE;
            head = block;
            printf("Allocated space at %p or %p\n", ret, (char*)block + SIZE_T_SIZE);
            return ret;
            // ret = block;
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
	return NULL;
}

void *os_realloc(void *ptr, size_t size)
{
	/* TODO: Implement os_realloc */
	return NULL;
}
