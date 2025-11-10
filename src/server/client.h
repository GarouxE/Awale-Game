#ifndef CLIENT_H
#define CLIENT_H

#include "server.h"
#include "message.h"

typedef enum {
   AVAILABLE,
   WAITING,
   IN_GAME
} Status;
typedef struct Client
{
   SOCKET sock;
   char name[BUF_SIZE];
   char bio[BUF_SIZE];
   Status status;
   struct Client* challenger; //pointer towards challenger
   MessageQueue* queue;
}Client;



#endif /* guard */
