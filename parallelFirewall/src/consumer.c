// SPDX-License-Identifier: BSD-3-Clause

#include <pthread.h>
#include <fcntl.h>
#include <unistd.h>
#include <semaphore.h>

#include "consumer.h"
#include "ring_buffer.h"
#include "packet.h"
#include "utils.h"

int currentThread = 0;

void consumer_thread(so_consumer_ctx_t *ctx)
{
	/* TODO: implement consumer thread */
    // pthread_mutex_lock(&ctx->producer_rb->mutexRing);
    // while ( ctx->producer_rb->imDone == 0 && ctx->producer_rb->len == 0 ) {
    //     pthread_cond_wait(&ctx->producer_rb->condRing, &ctx->producer_rb->mutexRing);
    // }
    // pthread_mutex_unlock(&ctx->producer_rb->mutexRing);
    while(1) {

        pthread_mutex_lock(ctx->mutexSync);
        // printf("Thread %d started\n", ctx->threadNum);
        while (ctx->threadNum != currentThread) {
            // printf("Thread %d waiting\n", ctx->threadNum);
            pthread_cond_wait(ctx->condConsumer, ctx->mutexSync);
        }
        pthread_mutex_unlock(ctx->mutexSync);
        so_packet_t packet;
        // printf("Thread %d is processing\n", ctx->threadNum);
        int ret = ring_buffer_dequeue(ctx->producer_rb, &packet, sizeof(so_packet_t));
        if ( ret == -1 ) {
            long unsigned int hash = packet_hash(&packet);
            so_action_t action = process_packet(&packet);


            pthread_mutex_lock(ctx->mutexConsumer);

            FILE *out = fopen(ctx->file, "a");
            fprintf(out, "%s %016lx %lu\n", RES_TO_STR(action), hash, packet.hdr.timestamp);
            fclose(out);
            pthread_mutex_unlock(ctx->mutexConsumer);
        }

        pthread_mutex_lock(ctx->mutexSync);

        // currentThread = (currentThread + 1) % ctx->nrThreads;
        currentThread++;
        currentThread %= ctx->nrThreads;
        pthread_cond_broadcast(ctx->condConsumer);

        // printf("Next Thread is : Thread %d\n", currentThread);
        pthread_mutex_unlock(ctx->mutexSync);

        if (ctx->producer_rb->imDone == 1 && ctx->producer_rb->len == 0) {
            // printf("Thread %d finished\n", ctx->threadNum);
            return;
        }
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
    // Empty file
    FILE *out = fopen(out_filename, "w");
    fclose(out);

    pthread_mutex_init(&mutex, NULL);
    pthread_mutex_init(&mutexC, NULL);
    pthread_mutex_init(&mutex2, NULL);
    pthread_cond_init(&condConsumer, NULL);
    rb->nrThreads = num_consumers;
	for (int i = 0; i < num_consumers; i++) {
		/*
		 * TODO: Launch consumer threads
		 **/
        so_consumer_ctx_t* ctx = malloc(sizeof(so_consumer_ctx_t));
        ctx->producer_rb = rb;
        ctx->nrThreads = num_consumers;
        ctx->threadNum = i;

        ctx->file = out_filename;
        ctx->mutexConsumer = &mutexC;
        ctx->mutexSync = &mutex;
        ctx->condConsumer = &condConsumer;
        ctx->mutexEnd = &mutex2;
        pthread_create(&tids[i], NULL, (void *)consumer_thread, ctx);

	}


	return num_consumers;
}
