/* SPDX-License-Identifier: BSD-3-Clause */

#ifndef __SO_RINGBUFFER_H__
#define __SO_RINGBUFFER_H__

#include <sys/types.h>
#include <string.h>

#include <stdatomic.h>

typedef struct so_ring_buffer_t {
	atomic_char *data;

	atomic_size_t read_pos;
	atomic_size_t write_pos;

	size_t len;
	size_t cap;

	/* TODO: Add syncronization primitives */
    pthread_mutex_t *mutexRing;
    pthread_cond_t *condRing, *condRing2;
    pthread_barrier_t *barrier;

    atomic_int imDone;
} so_ring_buffer_t;

int     ring_buffer_init(so_ring_buffer_t *rb, size_t cap);
ssize_t ring_buffer_enqueue(so_ring_buffer_t *rb, void *data, size_t size);
ssize_t ring_buffer_dequeue(so_ring_buffer_t *rb, void *data, size_t size);
void    ring_buffer_destroy(so_ring_buffer_t *rb);
void    ring_buffer_stop(so_ring_buffer_t *rb);

#endif /* __SO_RINGBUFFER_H__ */
