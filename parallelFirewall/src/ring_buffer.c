// SPDX-License-Identifier: BSD-3-Clause
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include "ring_buffer.h"


int iReadData = 0;

int ring_buffer_init(so_ring_buffer_t *ring, size_t cap)
{
	/* TODO: implement ring_buffer_init */
    ring->data = (char *)malloc(cap);
    ring->read_pos = 0;
    ring->write_pos = 0;
    ring->len = 0;
    ring->cap = cap;
    ring->imDone = 0;

    pthread_mutex_init(&ring->mutexRing, NULL);
    pthread_cond_init(&ring->condRing, NULL);
    // pthread_barrier_init(&ring->barrier, NULL, 2);


	return 1;
}

ssize_t ring_buffer_enqueue(so_ring_buffer_t *ring, void *data, size_t size)
{
	/* TODO: implement ring_buffer_enqueue */

    pthread_mutex_lock(&ring->mutexRing);
    // printf("Enqueue\n");
    iReadData++;
    memcpy(ring->data + ring->write_pos, data, size);
    ring->write_pos += size;
    ring->len += size;
    // pthread_cond_signal(&ring->condRing);

    pthread_mutex_unlock(&ring->mutexRing);

	return -1;
}

ssize_t ring_buffer_dequeue(so_ring_buffer_t *ring, void *data, size_t size)
{
	/* TODO: Implement ring_buffer_dequeue */
    pthread_mutex_lock(&ring->mutexRing);
    int p = 0;
    while ( iReadData == 0 && ring->len != 0 )
        p++;
        // pthread_cond_wait(&ring->condRing, &ring->mutexRing);

    iReadData--;
    memcpy(data, ring->data + ring->read_pos, size);
    ring->read_pos += size;
    ring->len -= size;

    pthread_mutex_unlock(&ring->mutexRing);

	return -1;
}

void ring_buffer_destroy(so_ring_buffer_t *ring)
{
	/* TODO: Implement ring_buffer_destroy */
    free(ring->data);

    // pthread_mutex_destroy(&ring->mutexRing);
    // pthread_cond_destroy(&ring->condRing);
    // pthread_barrier_destroy(&ring->barrier);


}

void ring_buffer_stop(so_ring_buffer_t *ring)
{
	/* TODO: Implement ring_buffer_stop */
    pthread_mutex_lock(&ring->mutexRing);

    ring->imDone = 1;

    pthread_mutex_unlock(&ring->mutexRing);

}
