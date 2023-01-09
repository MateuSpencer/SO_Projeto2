#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#include "producer-consumer.h"

int pcq_create(pc_queue_t *queue, size_t capacity) {
    // Allocate memory for the queue buffer
    queue->pcq_buffer = malloc(capacity * sizeof(void*));
    if (queue->pcq_buffer == NULL) {
        return -1;
    }
    //Initialize fields of the queue structure
    queue->pcq_capacity = capacity;
    queue->pcq_current_size = 0;
    queue->pcq_head = 0;
    queue->pcq_tail = 0;
    // Initialize the mutexes and condition variables
    pthread_mutex_init(&queue->pcq_current_size_lock, NULL);
    pthread_mutex_init(&queue->pcq_head_lock, NULL);
    pthread_mutex_init(&queue->pcq_tail_lock, NULL);
    pthread_mutex_init(&queue->pcq_pusher_condvar_lock, NULL);
    pthread_cond_init(&queue->pcq_pusher_condvar, NULL);
    pthread_mutex_init(&queue->pcq_popper_condvar_lock, NULL);
    pthread_cond_init(&queue->pcq_popper_condvar, NULL);

    return 0;
}

int pcq_destroy(pc_queue_t *queue) {
    // Free the queue buffer
    free(queue->pcq_buffer);
    // Destroy the mutexes and condition variables
    pthread_mutex_destroy(&queue->pcq_current_size_lock);
    pthread_mutex_destroy(&queue->pcq_head_lock);
    pthread_mutex_destroy(&queue->pcq_tail_lock);
    pthread_mutex_destroy(&queue->pcq_pusher_condvar_lock);
    pthread_cond_destroy(&queue->pcq_pusher_condvar);
    pthread_mutex_destroy(&queue->pcq_popper_condvar_lock);
    pthread_cond_destroy(&queue->pcq_popper_condvar);

    return 0;
}