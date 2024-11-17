// SPDX-License-Identifier: BSD-3-Clause

#include <pthread.h>
#include <fcntl.h>
#include <unistd.h>
#include <semaphore.h>

#include <stdatomic.h>

#include "consumer.h"
#include "ring_buffer.h"
#include "packet.h"
#include "utils.h"

static int currentThread;
atomic_int curThread = 0;

void consumer_thread(so_consumer_ctx_t *ctx)
{
	while (1) {
		pthread_mutex_lock(ctx->mutexEnd);
		while (ctx->threadNum != curThread)
			pthread_cond_wait(ctx->condConsumer, ctx->mutexEnd);
		pthread_mutex_unlock(ctx->mutexEnd);
		so_packet_t packet;
		int ret = ring_buffer_dequeue(ctx->producer_rb, &packet, sizeof(so_packet_t));
        printf("Thread %d no longer waiting\n", ctx->threadNum);
		if (ret == -1) {
			unsigned long hash = packet_hash(&packet);
			so_action_t action = process_packet(&packet);

			// pthread_mutex_lock(ctx->mutexConsumer);
			FILE *out = fopen(ctx->file, "a");

			fprintf(out, "%s %016lx %lu\n", RES_TO_STR(action), hash, packet.hdr.timestamp);
			fclose(out);
			// pthread_mutex_unlock(ctx->mutexConsumer);
		}
		// pthread_mutex_lock(ctx->mutexSync);
		curThread++;
		curThread %= ctx->nrThreads;
		// pthread_mutex_unlock(ctx->mutexSync);
		pthread_cond_broadcast(ctx->condConsumer);
		pthread_mutex_lock(ctx->producer_rb->mutexRing);
        printf("Thread %d. Im done: %d. ReaD: %d. Write: %d\n", ctx->threadNum, ctx->producer_rb->imDone, ctx->producer_rb->read_pos, ctx->producer_rb->write_pos);
		if (ctx->producer_rb->imDone == 1 && ctx->producer_rb->read_pos == ctx->producer_rb->write_pos) {
			pthread_mutex_unlock(ctx->producer_rb->mutexRing);
			return;
		}
		pthread_mutex_unlock(ctx->producer_rb->mutexRing);
	}
}
pthread_mutex_t mutex, mutex2;
pthread_mutex_t mutexC;
pthread_cond_t condConsumer;

int create_consumers(pthread_t *tids,
					 int num_consumers,
					 struct so_ring_buffer_t *rb,
					 const char *out_filename)
{
	FILE *out = fopen(out_filename, "w");

	fclose(out);
	pthread_mutex_init(&mutex, NULL);
	pthread_mutex_init(&mutexC, NULL);
	pthread_cond_init(&condConsumer, NULL);
	for (int i = 0; i < num_consumers; i++) {
		so_consumer_ctx_t *ctx = malloc(sizeof(so_consumer_ctx_t));

		ctx->producer_rb = rb;
		ctx->nrThreads = num_consumers;
		ctx->threadNum = i;
		ctx->file = out_filename;
		ctx->mutexConsumer = &mutexC;
		ctx->mutexSync = &mutex;
		ctx->condConsumer = &condConsumer;
        pthread_mutex_t *mutex3 = malloc(sizeof(pthread_mutex_t));
	    pthread_mutex_init(mutex3, NULL);
		ctx->mutexEnd = mutex3;
		pthread_create(&tids[i], NULL, (void *)consumer_thread, ctx);
	}
	return num_consumers;
}
