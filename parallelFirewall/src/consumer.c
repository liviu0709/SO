// SPDX-License-Identifier: BSD-3-Clause

#include <pthread.h>
#include <fcntl.h>
#include <unistd.h>
#include <semaphore.h>

#include "consumer.h"
#include "ring_buffer.h"
#include "packet.h"
#include "utils.h"

static int currentThread, currentPrint;

void consumer_thread(so_consumer_ctx_t *ctx)
{
	while (1) {
		pthread_mutex_lock(ctx->mutexSync);
		while (ctx->threadNum != currentThread)
			pthread_cond_wait(ctx->condConsumer, ctx->mutexSync);
		so_packet_t packet;
		int ret = ring_buffer_dequeue(ctx->producer_rb, &packet, sizeof(so_packet_t));

		currentThread++;
		currentThread %= ctx->nrThreads;
		pthread_mutex_unlock(ctx->mutexSync);
		pthread_cond_broadcast(ctx->condConsumer);
		if (ret == -1) {
			unsigned long hash = packet_hash(&packet);
			so_action_t action = process_packet(&packet);

			pthread_mutex_lock(ctx->mutexPrint);
			while (ctx->threadNum != currentPrint)
				pthread_cond_wait(ctx->condPrint, ctx->mutexPrint);
			FILE *out = fopen(ctx->file, "a");

			fprintf(out, "%s %016lx %lu\n", RES_TO_STR(action), hash, packet.hdr.timestamp);
			fclose(out);
			currentPrint++;
			currentPrint %= ctx->nrThreads;
			pthread_mutex_unlock(ctx->mutexPrint);
			pthread_cond_broadcast(ctx->condPrint);
		}
		if (ctx->producer_rb->imDone == 1 && ctx->producer_rb->read_pos == ctx->producer_rb->write_pos)
			return;
	}
}
pthread_mutex_t mutex, mutexPrint;

pthread_cond_t condConsumer, condPrint;

int create_consumers(pthread_t *tids,
					 int num_consumers,
					 struct so_ring_buffer_t *rb,
					 const char *out_filename)
{
	FILE *out = fopen(out_filename, "w");

	fclose(out);
	pthread_mutex_init(&mutex, NULL);
	pthread_mutex_init(&mutexPrint, NULL);
	pthread_cond_init(&condConsumer, NULL);
	pthread_cond_init(&condPrint, NULL);
so_consumer_ctx_t *ctxx = malloc(sizeof(so_consumer_ctx_t) * num_consumers);
	for (int i = 0; i < num_consumers; i++) {
		so_consumer_ctx_t *ctx = &ctxx[i];

		ctx->producer_rb = rb;
		ctx->nrThreads = num_consumers;
		ctx->threadNum = i;
		ctx->file = out_filename;
		ctx->mutexSync = &mutex;
		ctx->condConsumer = &condConsumer;
		ctx->condPrint = &condPrint;
		ctx->mutexPrint = &mutexPrint;
		pthread_create(&tids[i], NULL, (void *)consumer_thread, ctx);
	}
rb->ctx = ctxx;
	return num_consumers;
}
