#ifndef CLIENT_H
#define CLIENT_H

#include "server.h"
#include "message.h"

typedef enum {
   AVAILABLE,
   WAITING,
   IN_GAME,
   OBSERVING
} Status;
typedef struct Client
{
   SOCKET sock;
   char name[BUF_SIZE];
   int score;
   char bio[BUF_SIZE];
   Status status;
   struct Client* challenger; //pointer towards challenger
   MessageQueue* queue;
   int game_location;
   /* Privacy: allow player to restrict observers to a list of friends */
   int private_mode; /* 0 = public (default), 1 = private */
#define MAX_FRIENDS 16
   char friends[MAX_FRIENDS][BUF_SIZE];
   int friend_count;
}Client;



#endif /* guard */
