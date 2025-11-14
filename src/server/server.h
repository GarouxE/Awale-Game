#ifndef SERVER_H
#define SERVER_H

#ifdef WIN32

#include <winsock2.h>

#elif defined (linux)

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h> /* close */
#include <signal.h>
#include <netdb.h> /* gethostbyname */
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#define closesocket(s) close(s)
typedef int SOCKET;
typedef struct sockaddr_in SOCKADDR_IN;
typedef struct sockaddr SOCKADDR;
typedef struct in_addr IN_ADDR;

#else

#error not defined for this platform

#endif

#define CRLF        "\r\n"
#define PORT         1977
#define MAX_CLIENTS     100
#define MAX_GAMES       100
#define BUF_SIZE    1024
#define BUF_VIEWERS 10

#include "client.h"
#include "../game/game.h" 

typedef struct {
   Board *board;
   Client player1;
   Client player2;
   char game_name[BUF_SIZE];
   Client *viewers[BUF_VIEWERS];
   int nb_viewers;
} Game;
typedef struct {
   Game** games;
   Client* clients;
   int actual;
   Game* current_game;
} Context;

/* Only expose functions that are used by other modules. Internal helpers
   are defined static inside server.c and should not be declared here. */

/* write_client is implemented in server.c and used by other modules. */
void write_client(SOCKET sock, const char *buffer);


#endif /* guard */
