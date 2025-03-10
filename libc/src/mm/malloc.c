// SPDX-License-Identifier: BSD-3-Clause

#include <internal/mm/mem_list.h>
#include <internal/types.h>
#include <internal/essentials.h>
#include <sys/mman.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

typedef struct {
    long long size;
} BlockSize;

void *malloc(size_t size)
{
	/* TODO: Implement malloc(). */
    // The size of the block has to be saved
    long long realSize = size + sizeof(BlockSize);
    // The call on the next line appears upon using malloc from libc... so copy it
    // mmap(NULL, 692224, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0) = 0x7ff296080000
    BlockSize *ret = mmap(NULL, realSize, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	if ( ret == MAP_FAILED ) {
        return NULL;
    }
    ret->size = realSize;
    return (void*)ret + sizeof(BlockSize);
}

void *calloc(size_t nmemb, size_t size)
{
	/* TODO: Implement calloc(). */
    // calloc is similar to malloc except size * nmemb
    long long realSize = size * nmemb + sizeof(BlockSize);
	BlockSize *ret = mmap(NULL, realSize, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if ( ret == MAP_FAILED )
        return NULL;
    ret->size = realSize;
    return (void*)ret + sizeof(BlockSize);
}

void free(void *ptr)
{
	/* TODO: Implement free(). */
    BlockSize *data = ((BlockSize*)(((char*)ptr - sizeof(BlockSize))));
    munmap(ptr - sizeof(BlockSize), data->size);
}

void *realloc(void *ptr, size_t size)
{
    BlockSize *data = ((BlockSize*)(((char*)ptr - sizeof(BlockSize))));
	/* TODO: Implement realloc(). */
    long long realSize = size + sizeof(BlockSize);
    long long old = data->size;
    BlockSize *ret = mmap(NULL, realSize , PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    memcpy(ret + sizeof(BlockSize), ptr, size);
    munmap(ptr - sizeof(BlockSize), old);
	ret->size = realSize;
    return (void*)ret + sizeof(BlockSize);
}

void *reallocarray(void *ptr, size_t nmemb, size_t size)
{
	/* TODO: Implement reallocarray(). */
    BlockSize *data = ((BlockSize*)(((char*)ptr - sizeof(BlockSize))));
	/* TODO: Implement realloc(). */
    long long realSize = size * nmemb + sizeof(BlockSize);
    long long old = data->size;
    BlockSize *ret = mmap(NULL, realSize , PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    memcpy(ret + sizeof(BlockSize), ptr, size * nmemb);
    munmap(ptr - sizeof(BlockSize), old);
	ret->size = realSize;
    return (void*)ret + sizeof(BlockSize);
}
