// SPDX-License-Identifier: BSD-3-Clause
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include "ring_buffer.h"


pthread_mutex_t mutexRing;
pthread_cond_t condRing, condRing2;

static int iReadData;

int ring_buffer_init(so_ring_buffer_t *ring, size_t cap)
{
	ring->data = malloc(cap);
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
	return 1;
}

ssize_t ring_buffer_enqueue(so_ring_buffer_t *ring, void *data, size_t size)
{
	if (ring->read_pos == ring->write_pos) {
		ring->read_pos = 0;
		ring->write_pos = 0;
		ring->len = 0;
	}
	pthread_mutex_lock(ring->mutexRing);
	while (ring->write_pos + size > ring->cap)
		pthread_cond_wait(ring->condRing2, ring->mutexRing);
	pthread_mutex_unlock(ring->mutexRing);
	memcpy(ring->data + ring->write_pos, data, size);
	ring->write_pos += size;
	pthread_cond_signal(ring->condRing);
	return -1;
}

ssize_t ring_buffer_dequeue(so_ring_buffer_t *ring, void *data, size_t size)
{
	pthread_mutex_lock(ring->mutexRing);
	while (ring->write_pos == ring->read_pos && ring->imDone == 0)
		pthread_cond_wait(ring->condRing, ring->mutexRing);
	if (ring->write_pos == ring->read_pos && ring->imDone == 1) {
		pthread_mutex_unlock(ring->mutexRing);
		return 0;
	}
	memcpy(data, ring->data + ring->read_pos, size);
	ring->read_pos += size;
	if (ring->read_pos == ring->write_pos) {
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
	free(ring->data);
	pthread_mutex_destroy(ring->mutexRing);
	pthread_cond_destroy(ring->condRing);
	pthread_cond_destroy(ring->condRing2);
}

void ring_buffer_stop(so_ring_buffer_t *ring)
{
	pthread_mutex_lock(ring->mutexRing);
	ring->imDone = 1;
	pthread_cond_broadcast(ring->condRing);
	pthread_mutex_unlock(ring->mutexRing);
}
