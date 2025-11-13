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

static void init(void);
static void end(void);
static void app(void);
static int init_connection(void);
static void end_connection(int sock);
void* play_thread(void* arg);
static int read_client(SOCKET sock, char *buffer);
static void write_client(SOCKET sock, const char *buffer);
static void send_message_to_all_clients(Client *clients, Client client, int actual, const char *buffer, char from_server);
static void list_clients(Client *clients, int actual, char* response);
static void list_games(Game **games, char* response);
static void modify_bio(Client* sender, char* buffer, char* response);
static int challenge_player(Client *clientList, Client* client, int actual, char *buffer);
static void list_commands(Client* client, char* response);
static void talk_to(Client* clients, Client* sender, int actual, char* buffer, char* response);
static void accept_challenge(Game** games, Client* clientList, Client* challengee, char* response, int actual);
static void refuse_challenge(Client* challengee, char* response);
static void observe_game(Game** games, Client* clients, int actual, char* buffer, Client* client, char* response);
static void quit_game(Game** games, Client* sender, char* response );
int play(Game** games, Client* clients, Client player1, int actual, Client player2, Game* game);
static void print_board(Board* board, char* buffer, Client player1, Client player2);
static void consult_client(Client *clients, int actual, char*buffer, char* response);
static void remove_client(Client *clients, int to_remove, int *actual); 
static void clear_clients(Client *clients, int actual);
static void parse_command(const char *buffer, char* username, char* message, int username_bool, int message_bool);
static void treat_command(Game **games, Client *clients, Client* client, int actual, const char *buffer, int in_game);
static int create_game(Client *player1, Client *player2, Game **gamelist, Board *board, Game *new_game);
int save_game(Client *player1, Client *player2, Board *board, Game *game);
static int remove_game(Game **gamelist, Game *game);


#endif /* guard */
