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
        printf("Checking block at %p with size %d\n", current, size);
        if ( current->status == STATUS_FREE && current->size >= size ) {
            return current;
        }
        current = current->prev;
    }
    return NULL;
}

void *os_malloc(size_t size)
{
	/* TODO: Implement os_malloc */
    if ( size == 0 )
        return NULL;
    if ( size >= MMAP_THRESHOLD ) {
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

    } else {
        struct block_meta *current = head;
        void *blk, *ret;
        if ( heapStart == NULL ) {
            // Preallocating memory
            printf("Preallocating memory\n");
            heapStart = sbrk(MMAP_THRESHOLD);
            blk = heapStart;
            struct block_meta *block = blk;
            block->size = size;
            block->status = STATUS_ALLOC;
            if ( head == NULL ) {
                head = block;
                head->next = NULL;
                head->prev = NULL;
                head->status = STATUS_ALLOC;
                head->size = size;
            } else {
                head->next = block;
                block->prev = head;
                block->next = NULL;
                block->status = STATUS_ALLOC;
                block->size = size;
                head = block;
            }
            ret = blk + SIZE_T_SIZE;
            // Make a new block with the remaining memory
            // Check first if there is enough space for a new block
            if ( (int)(MMAP_THRESHOLD - size - SIZE_T_SIZE * 2) >= 0 ) {
                blk += SIZE_T_SIZE + size;
                struct block_meta *block2 = blk;
                block2->size = MMAP_THRESHOLD - size - SIZE_T_SIZE * 2;
                block2->status = STATUS_FREE;
                block2->prev = head;
                block2->next = NULL;
                head->next = block2;
                head = block2;
            }
        } else {
            // Find a free block with enough space
            struct block_meta *block = lfGudBlock(size);
            if ( block != NULL ) {
                // Split the block for the size we need
                if ( block->size > size + SIZE_T_SIZE * 2 ) {
                    struct block_meta *block2 = block + SIZE_T_SIZE + size;
                    block2->size = block->size - size - SIZE_T_SIZE;
                    block2->status = STATUS_FREE;
                    block2->prev = block;
                    block2->next = block->next;
                    block->size = size;
                    if ( block->next == NULL )
                        head = block2;
                    block->next = block2;
                }
                block->status = STATUS_ALLOC;
                ret = block;
                ret += SIZE_T_SIZE;
                printf("Allocated %ld bytes at %p\n", size, ret);
            } else {
                printf("We fucked up...%d\n", debug);
                debug++;
                // Get space for another block...
                struct block_meta *block = sbrk(ALIGN(size + SIZE_T_SIZE));
                // struct block_meta *block = sbrk(size);
                block->prev = head;
                block->size = size;
                block->status = STATUS_ALLOC;
                head->next = block;
                ret = block;
                // Pointer aritmetic sucks...
                ret += SIZE_T_SIZE;
                head = block;
                // ret = block;
            }
        }
        printf("allocated space at %p\n", ret);
        return ret;
    }
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
