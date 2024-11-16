/* SPDX-License-Identifier: BSD-3-Clause */

#ifndef __SO_CONSUMER_H__
#define __SO_CONSUMER_H__


#include <pthread.h>
#include <fcntl.h>
#include <unistd.h>
#include <semaphore.h>

#include "consumer.h"


#include "utils.h"
#include "ring_buffer.h"
#include "packet.h"

typedef struct so_consumer_ctx_t {
	struct so_ring_buffer_t *producer_rb;

    /* TODO: add synchronization primitives for timestamp ordering */
    pthread_mutex_t *mutexConsumer;
    pthread_mutex_t *mutexSync;
    const char* file;
    int threadNum;
    int nrThreads;
    pthread_cond_t *condConsumer;
    pthread_mutex_t* mutexEnd;
} so_consumer_ctx_t;

int create_consumers(pthread_t *tids,
					int num_consumers,
					so_ring_buffer_t *rb,
					const char *out_filename);

#endif /* __SO_CONSUMER_H__ */
