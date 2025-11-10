#ifndef MESSAGE_H
#define MESSAGE_H

#include <pthread.h>

#define BUF_SIZE 1024
#define MAX_MESSAGE 100

typedef struct Message
{
    int client_sock;
    char content[BUF_SIZE];
} Message;

typedef struct MessageQueue
{
  Message messages[MAX_MESSAGE];
  int front;
  int rear;
  pthread_mutex_t lock;
  pthread_cond_t not_empty;  
} MessageQueue;

void queue_push(MessageQueue *queue, Message msg);
Message queue_pop(MessageQueue *queue);

#endif