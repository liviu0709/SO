// SPDX-License-Identifier: BSD-3-Clause

#include <pthread.h>
#include <fcntl.h>
#include <unistd.h>
#include <semaphore.h>

#include "consumer.h"
#include "ring_buffer.h"
#include "packet.h"
#include "utils.h"

void consumer_thread(so_consumer_ctx_t *ctx)
{
	/* TODO: implement consumer thread */
    pthread_mutex_lock(&ctx->mutexConsumer);
    while(1) {
        so_packet_t packet;

        ring_buffer_dequeue(ctx->producer_rb, &packet, sizeof(so_packet_t));
        long unsigned int hash = packet_hash(&packet);
        so_action_t action = process_packet(&packet);
        FILE *out = fopen(ctx->file, "a");
        fprintf(out, "%s %016lx %lu\n", RES_TO_STR(action), hash, packet.hdr.timestamp);
        fclose(out);
        if (ctx->producer_rb->imDone == 1 && ctx->producer_rb->len == 0) {
            pthread_mutex_unlock(&ctx->mutexConsumer);
            return;
        }
    }

    pthread_mutex_unlock(&ctx->mutexConsumer);


}

int create_consumers(pthread_t *tids,
					 int num_consumers,
					 struct so_ring_buffer_t *rb,
					 const char *out_filename)
{
    // Empty file
    FILE *out = fopen(out_filename, "w");
    fclose(out);

	for (int i = 0; i < num_consumers; i++) {
		/*
		 * TODO: Launch consumer threads
		 **/
        so_consumer_ctx_t* ctx = malloc(sizeof(so_consumer_ctx_t));
        ctx->producer_rb = rb;


        ctx->file = out_filename;
        pthread_mutex_init(&ctx->mutexConsumer, NULL);
        pthread_create(&tids[i], NULL, (void *)consumer_thread, ctx);

	}


	return num_consumers;
}
