#ifndef CLIENT_H
#define CLIENT_H

#include "server.h"
#include "message.h"

typedef enum {
   AVAILABLE,
   WAITING,
   WAITING_PRIVATE,
   WAITING_PUBLIC,
   IN_GAME,
   OBSERVING
} Status;


typedef struct Client
{
   SOCKET sock;
   char name[BUF_SIZE];
   char bio[BUF_SIZE];
   Status status;
   struct Client* challenger; //pointer towards challenger
   MessageQueue* queue;
   int game_location;
}Client;



#endif /* guard */
