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
atomic_int curThreadPrint = 0;
atomic_int imNotOut = 0;
void consumer_thread(so_consumer_ctx_t *ctx)
{
	while (1) {

        pthread_mutex_lock(ctx->mutexConsumer);
        while (ctx->threadNum != curThread && imNotOut == 0 ) {
            pthread_cond_wait(ctx->condConsumer, ctx->mutexConsumer);
        }
        curThread = (curThread + 1) % ctx->nrThreads;
        so_packet_t packet;
        pthread_mutex_unlock(ctx->mutexConsumer);
		int ret = ring_buffer_dequeue(ctx->producer_rb, &packet, sizeof(so_packet_t));
        pthread_cond_broadcast(ctx->condConsumer);
		if (ret == -1) {
			unsigned long hash = packet_hash(&packet);
			so_action_t action = process_packet(&packet);
            pthread_mutex_lock(ctx->mutexSync);
            while (ctx->threadNumPrint != curThreadPrint) {
                pthread_cond_wait(ctx->condPrint, ctx->mutexSync);
            }
            curThreadPrint = (curThreadPrint + 1) % ctx->nrThreads;
			fprintf(ctx->out, "%s %016lx %lu\n", RES_TO_STR(action), hash, packet.hdr.timestamp);
            pthread_mutex_unlock(ctx->mutexSync);
            pthread_cond_broadcast(ctx->condPrint);
        }
        // pthread_mutex_lock(ctx->producer_rb->mutexRing);
        // printf("Thread %d. Read %d. Write %d\n", ctx->threadNum, ctx->producer_rb->read_pos, ctx->producer_rb->write_pos);
        if ( ctx->producer_rb->imDone == 1 && ctx->producer_rb->write_pos == ctx->producer_rb->read_pos) {
            // printf("Thread %d finished\n", ctx->threadNum);
            // pthread_mutex_unlock(ctx->producer_rb->mutexRing);
            imNotOut = 1;
            break;
        }
        // pthread_mutex_unlock(ctx->producer_rb->mutexRing);
	}
}
pthread_mutex_t mutex, mutex2;
pthread_mutex_t mutexC;
pthread_cond_t condConsumer, print;

int create_consumers(pthread_t *tids,
					 int num_consumers,
					 struct so_ring_buffer_t *rb,
					 const char *out_filename)
{
	FILE *out = fopen(out_filename, "w");
    rb->out = out;
	// fclose(out);
	pthread_mutex_init(&mutex, NULL);
	pthread_mutex_init(&mutexC, NULL);
	pthread_cond_init(&condConsumer, NULL);
    pthread_cond_init(&print, NULL);
	for (int i = 0; i < num_consumers; i++) {
		so_consumer_ctx_t *ctx = malloc(sizeof(so_consumer_ctx_t));

		ctx->producer_rb = rb;
		ctx->nrThreads = num_consumers;
		ctx->threadNum = i;
		ctx->file = out_filename;
		ctx->mutexConsumer = &mutexC;
		ctx->mutexSync = &mutex;
		ctx->condConsumer = &condConsumer;
        ctx->condPrint = &print;
        ctx->out = out;
        ctx->threadNumPrint = i;
        pthread_mutex_t *mutex3 = malloc(sizeof(pthread_mutex_t));
	    pthread_mutex_init(mutex3, NULL);
		ctx->mutexEnd = mutex3;
		pthread_create(&tids[i], NULL, (void *)consumer_thread, ctx);
	}
	return num_consumers;
}
