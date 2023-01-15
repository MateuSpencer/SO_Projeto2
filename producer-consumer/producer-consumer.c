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

int pcq_enqueue(pc_queue_t *queue, void *elem) {
    pthread_mutex_lock(&queue->pcq_current_size_lock);
    // If the queue is full, wait on the condition variable until the queue has space
    while (queue->pcq_current_size == queue->pcq_capacity) {
        pthread_cond_wait(&queue->pcq_pusher_condvar, &queue->pcq_current_size_lock);
    }
    // Enqueue the element at the tail of the queue
    pthread_mutex_lock(&queue->pcq_tail_lock);
    queue->pcq_buffer[queue->pcq_tail] = elem;
    //update size and head of queue (% to wrap arround when it is the last index)
    queue->pcq_tail = (queue->pcq_tail + 1) % queue->pcq_capacity;
    queue->pcq_current_size++;
    pthread_mutex_unlock(&queue->pcq_tail_lock);
    // Signal popper condition variable to wake up threads that are waiting to dequeue an element
    pthread_cond_signal(&queue->pcq_popper_condvar);
    pthread_mutex_unlock(&queue->pcq_current_size_lock);
    return 0;
}

void *pcq_dequeue(pc_queue_t *queue) {
    void *elem;
    // If the queue is empty, wait on the condition variable, until the queue has an element
    pthread_mutex_lock(&queue->pcq_current_size_lock);
    while (queue->pcq_current_size == 0) {
        pthread_cond_wait(&queue->pcq_popper_condvar, &queue->pcq_current_size_lock);
    }
    // Dequeue the element at the current head position
    pthread_mutex_lock(&queue->pcq_head_lock);
    elem = queue->pcq_buffer[queue->pcq_head];
    //update size and head of queue (% to wrap arround when it is the last index)
    queue->pcq_head = (queue->pcq_head + 1) % queue->pcq_capacity;
    queue->pcq_current_size--;
    pthread_mutex_unlock(&queue->pcq_head_lock);
    // Signal pusher condition variable to wake up threads that are waiting to enqueue an element
    pthread_cond_signal(&queue->pcq_pusher_condvar);
    pthread_mutex_unlock(&queue->pcq_current_size_lock);
    return elem;
}