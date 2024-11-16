// SPDX-License-Identifier: BSD-3-Clause
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include "ring_buffer.h"


pthread_mutex_t mutexRing;
pthread_cond_t condRing, condRing2;

int iReadData = 0;

int ring_buffer_init(so_ring_buffer_t *ring, size_t cap)
{
	/* TODO: implement ring_buffer_init */
    ring->data = (char *)malloc(cap);
    // ring->data = (char *)malloc(cap);
    ring->read_pos = 0;
    ring->write_pos = 0;
    ring->len = 0;
    ring->cap = cap;
    ring->imDone = 0;
    ring->mutexRing = &mutexRing;
    ring->condRing = &condRing;
    ring->condRing2 = &condRing2;
    pthread_cond_init(ring->condRing2, NULL);
    pthread_mutex_init(ring->mutexRing, NULL);
    pthread_cond_init(ring->condRing, NULL);
    // pthread_barrier_init(&ring->barrier, NULL, 2);


	return 1;
}

ssize_t ring_buffer_enqueue(so_ring_buffer_t *ring, void *data, size_t size)
{
	/* TODO: implement ring_buffer_enqueue */

    // printf("Enqueue\n");
    // iReadData++;
    // printf("Read %ld, write %ld\n", ring->read_pos, ring->write_pos);
    if ( ring->read_pos == ring->write_pos ) {
        ring->read_pos = 0;
        ring->write_pos = 0;
        ring->len = 0;

    }


    pthread_mutex_lock(ring->mutexRing);
    while ( ring->write_pos + size > ring->cap ) {
        // printf("Enqueue waiting..\n");
        pthread_cond_wait(ring->condRing2, ring->mutexRing);
    }
    pthread_mutex_unlock(ring->mutexRing);

    memcpy(ring->data + ring->write_pos, data, size);
    ring->write_pos += size;
    // ring->len += size;
    pthread_cond_signal(ring->condRing);
    // printf("Enqueue\n");

	return -1;
}

ssize_t ring_buffer_dequeue(so_ring_buffer_t *ring, void *data, size_t size)
{
	/* TODO: Implement ring_buffer_dequeue */
    pthread_mutex_lock(ring->mutexRing);

    // while( ring->imDone == 0 )
        // pthread_cond_wait(&ring->condRing, &ring->mutexRing);

    // int p = 0;
    while ( ring->write_pos == ring->read_pos && ring->imDone == 0 ) {
        // printf("Dequeue waiting..\n");
        pthread_cond_wait(ring->condRing, ring->mutexRing);
    }
    // printf("Deqeue Read %ld, write %ld\n", ring->read_pos, ring->write_pos);
    // printf("Dequeue\n");
    // iReadData--;

    if ( ring->write_pos == ring->read_pos && ring->imDone == 1 ) {
        pthread_mutex_unlock(ring->mutexRing);
        return 0;
    }


    memcpy(data, ring->data + ring->read_pos, size);
    ring->read_pos += size;
    if ( ring->read_pos == ring->write_pos ) {
        ring->read_pos = 0;
        ring->write_pos = 0;
        ring->len = 0;
        pthread_cond_signal(ring->condRing2);

    }

    pthread_mutex_unlock(ring->mutexRing);

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
    pthread_mutex_lock(ring->mutexRing);

    ring->imDone = 1;
    pthread_cond_broadcast(ring->condRing);
    // printf("all data read\n");
    pthread_mutex_unlock(ring->mutexRing);

}
