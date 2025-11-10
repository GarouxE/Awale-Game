#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "message.h"

void queue_push(MessageQueue *queue, Message msg) {
    pthread_mutex_lock(&queue->lock);
    queue->messages[queue->rear] = msg;
    queue->rear = (queue->rear + 1) % MAX_MESSAGE;
    pthread_cond_signal(&queue->not_empty);
    pthread_mutex_unlock(&queue->lock);
}

Message queue_pop(MessageQueue *queue) {
    pthread_mutex_lock(&queue->lock);
    while (queue->front == queue->rear) {
        pthread_cond_wait(&queue->not_empty, &queue->lock);
    }
    Message msg = queue->messages[queue->front];
    queue->front = (queue->front + 1) % MAX_MESSAGE;
    pthread_mutex_unlock(&queue->lock);
    return msg;
}