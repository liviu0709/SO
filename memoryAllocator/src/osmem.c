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

struct block_meta *head;

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
            head->status = STATUS_ALLOC;
        } else {
            head->next = adr;
            adr->prev = head;
            adr->next = NULL;
            adr->size = size;
            adr->status = STATUS_ALLOC;
            head = adr;
        }
        return blk + SIZE_T_SIZE;

    } else {
        return NULL;
    }
}

void os_free(void *ptr)
{
	/* TODO: Implement os_free */
    if ( ptr == NULL ) {
        return;
    }
    struct block_meta *block = ptr - SIZE_T_SIZE;
    munmap(ptr - SIZE_T_SIZE, ALIGN(block->size + SIZE_T_SIZE));
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
