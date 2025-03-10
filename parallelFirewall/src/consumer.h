/* SPDX-License-Identifier: BSD-3-Clause */

#ifndef __SO_CONSUMER_H__
#define __SO_CONSUMER_H__


#include <pthread.h>
#include <fcntl.h>
#include <unistd.h>
#include <semaphore.h>

#include "consumer.h"


#include <stdatomic.h>

#include "utils.h"
#include "ring_buffer.h"
#include "packet.h"

typedef struct so_consumer_ctx_t {
	struct so_ring_buffer_t *producer_rb;

/* TODO: add synchronization primitives for timestamp ordering */
pthread_mutex_t *mutexConsumer;
pthread_mutex_t *mutexSync;
pthread_mutex_t *mutexPrint;
const char* file;
int threadNum;
int threadNumPrint;
int nrThreads;
pthread_cond_t *condConsumer;
pthread_cond_t *condPrint;
pthread_mutex_t* mutexEnd;

FILE *out;
} so_consumer_ctx_t;

int create_consumers(pthread_t *tids,
					int num_consumers,
					struct so_ring_buffer_t *rb,
					const char *out_filename);

#endif /* __SO_CONSUMER_H__ */
