#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>

#include "server.h"
#include "../game/game.h"

static void init(void)
{
#ifdef WIN32
   WSADATA wsa;
   int err = WSAStartup(MAKEWORD(2, 2), &wsa);
   if(err < 0)
   {
      puts("WSAStartup failed !");
      exit(EXIT_FAILURE);
   }
#endif
}

static void end(void)
{
#ifdef WIN32
   WSACleanup();
#endif
}

static void app(void)
{
   SOCKET sock = init_connection();
   char buffer[BUF_SIZE];
   /* the index for the array */
   int actual = 0;
   int max = sock;
   /* an array for all clients */
   Client clients[MAX_CLIENTS];
   Game* games[MAX_GAMES];
   memset(games, 0, sizeof(games)); 

   fd_set rdfs;

   while(1)
   {
      int i = 0;
      FD_ZERO(&rdfs);

      /* add STDIN_FILENO */
      FD_SET(STDIN_FILENO, &rdfs);

      /* add the connection socket */
      FD_SET(sock, &rdfs);

      /* add socket of each client */
      for(i = 0; i < actual; i++)
      {
         FD_SET(clients[i].sock, &rdfs);
      }

      if(select(max + 1, &rdfs, NULL, NULL, NULL) == -1)
      {
         perror("select()");
         exit(errno);
      }

      /* something from standard input : i.e keyboard */
      if(FD_ISSET(STDIN_FILENO, &rdfs))
      {
         /* stop process when type on keyboard */
         break;
      }
      else if(FD_ISSET(sock, &rdfs))
      {
         /* new client */
         SOCKADDR_IN csin = { 0 };
         size_t sinsize = sizeof csin;
         int csock = accept(sock, (SOCKADDR *)&csin, &sinsize);
         if(csock == SOCKET_ERROR)
         {
            perror("accept()");
            continue;
         }

         /* after connecting the client sends its name */
         if(read_client(csock, buffer) == -1)
         {
            /* disconnected */
            continue;
         }

         /* what is the new maximum fd ? */
         max = csock > max ? csock : max;

         FD_SET(csock, &rdfs);

         Client c = { csock };
         strncpy(c.name, buffer, BUF_SIZE - 1);
         c.status = AVAILABLE;
         
         c.queue = malloc(sizeof(MessageQueue));
         memset(c.queue, 0, sizeof(MessageQueue));
         pthread_mutex_init(&c.queue->lock, NULL);
         pthread_cond_init(&c.queue->not_empty, NULL);
         c.queue->front = 0;
         c.queue->rear = 0;

         clients[actual] = c;
         actual++;
         printf("%s has joined.\n", c.name);
      }
      else
      {
         int i = 0;
         for(i = 0; i < actual; i++)
         {
            /* a client is talking */
            if(FD_ISSET(clients[i].sock, &rdfs))
            {
               Client* client = &clients[i];
               int c = read_client(clients[i].sock, buffer);
               /* client disconnected */
               if(c == 0)
               {
                  closesocket(clients[i].sock);
                  remove_client(clients, i, &actual);
                  strncpy(buffer, client->name, BUF_SIZE - 1);
                  strncat(buffer, " has left.", BUF_SIZE - strlen(buffer) - 1);
                  printf("%s has left.\n", client->name);
                  send_message_to_all_clients(clients, *client, actual, buffer, 1);
               }
               else {
                  // Si le client est en jeu, on ajoute à sa queue
                  if (client->status == IN_GAME) {
                     Message msg;
                     strncpy(msg.content, buffer, BUF_SIZE - 1);
                     msg.content[BUF_SIZE - 1] = '\0';
                     msg.client_sock = client->sock;
                     queue_push(client->queue, msg);
                  } else {
                     // Commande spéciale
                     if (buffer[0] == '/') {
                        printf("Commande reçue de %s : %s\n", client->name, buffer);
                        treat_command(games, clients, client, actual, buffer, 0);   
                     } else {
                        send_message_to_all_clients(clients, *client, actual, buffer, 0);
                     }
                  }
               }
               break;
            }
         }
      }
   }

   clear_clients(clients, actual);
   end_connection(sock);
}

static void clear_clients(Client *clients, int actual)
{
   int i = 0;
   for(i = 0; i < actual; i++)
   {
      closesocket(clients[i].sock);
   }
}

static int challenge_player(Client *clientList, Client* client, int actual, char *buffer) {
   char username[BUF_SIZE]; 
   char message[BUF_SIZE];
   parse_command(buffer, username, message, 1, 0);
   buffer[0] = '\0';

   if (client->status != AVAILABLE ) {
      snprintf(message, sizeof(message),
               "[ERROR] Answer to your last challenge invitation or end your current match");
      write_client(client->sock, message);
      return -1;
   } else if (!strcmp(client->name, username)) {
      snprintf(message, sizeof(message),
               "[ERROR] You cannot challenge yourself");
      write_client(client->sock, message);
      return -1;
   }

   // Check if the challenged user exists
   Client *challengee = NULL;
   for (int i = 0; i < actual; i++) {
      if (strcmp(clientList[i].name, username) == 0) {
         challengee = &clientList[i];
      } 
   }

   if (challengee == NULL) {
      snprintf(message, sizeof(message),
               "[ERROR] user not found");
      write_client(client->sock, message);
      return -1;
   } else if (challengee->status != AVAILABLE) {
      snprintf(message, sizeof(message),
               "[ERROR] user not available");
      write_client(client->sock, message);
      return -1;
   }

   snprintf(message, sizeof(message),"You've challenged %s !", challengee->name);
   write_client(client->sock, message);
   
   // Ask the target player if they accept the challenge
   snprintf(message, sizeof(message),"%s has challenged you! Do you accept? Type '/accept' or '/refuse'.", client->name);
   write_client(challengee->sock, message);

   challengee->status = WAITING;
   client->status = WAITING;
   challengee->challenger = client;

   return 0;
}



static void list_clients(Client *clients, int actual, char* response){
   int i = 0;
   strncat(response, "Here is the list of all users: ", BUF_SIZE - strlen(response) - 1);
   for (i =0; i<actual;i++){
      strncat(response, "\n - ", BUF_SIZE - strlen(response) - 1);
      strncat(response, clients[i].name, BUF_SIZE - strlen(response) - 1);
      strncat(response, clients[i].status==AVAILABLE ? "" : 
         clients[i].status == IN_GAME ? " (in game)" : " (waiting)",
         BUF_SIZE - strlen(response) - 1);
   }

}

static void list_games(Game **games, char* response){
   int i = 0;
   int count = 0;
   
   strncat(response, "Here is the list of ongoing matches: ", BUF_SIZE - strlen(response) - 1);
   for (i = 0; i<MAX_GAMES;i++){
      if (games[i] != NULL) {
         snprintf(response, BUF_SIZE - strlen(response) - 1, "\n - %d : ", i);
         if (games[i]->game_name[0] == '\0') {
            strncat(response, "[Unnamed game]", BUF_SIZE - strlen(response) - 1);
         } else {
            strncat(response, games[i]->game_name, BUF_SIZE - strlen(response) - 1);
            count++;
         }
      }
   }
   if (count == 0) strncat(response, "\nNo match is currently ongoing. ", BUF_SIZE - strlen(response) - 1); 

}

static void modify_bio(Client* sender, char* buffer, char* response) {
   char username[0];
   char new_bio[BUF_SIZE];
   parse_command(buffer, username, new_bio, 0, 1);

   if (!strcmp(new_bio,"")) {
      snprintf(response, BUF_SIZE - strlen(response) - 1,
               "[ERROR] new bio should not be empty ");
      return;
   }

   strncpy(sender->bio, new_bio, BUF_SIZE - 1);
   sender->bio[BUF_SIZE - 1] = '\0';
   strcpy(response, "[SUCCESS] Votre bio a été mise à jour.");
      
}

static void consult_client(Client *clients, int actual, char*buffer, char* response) {
   char username[BUF_SIZE];
   char message[0];
   parse_command(buffer, username, message, 1, 0);
   int found = 0;

   for (int i = 0; i < actual; i++) {
      if (!strcmp(clients[i].name, username)) {
         snprintf(response, BUF_SIZE,
                     "Nom : %s\nBio : %s",
                     clients[i].name,
                     clients[i].bio[0] ? clients[i].bio : "(Aucune bio)");

         found = 1;
         break;
      }
   }
   if (!found) {
      strcpy(response, "[ERROR] User not found.");
   }
}

static void print_board(Board* board, char* buffer, Client player1, Client player2) {

   buffer[0] = '\0';

   snprintf(buffer + strlen(buffer), BUF_SIZE - strlen(buffer),
      "\n========== GAME BOARD ==========\n"
      "          %s (P1)\n\n"
      "    A   B   C   D   E   F\n"
      "   (%d) (%d) (%d) (%d) (%d) (%d)\n"
      "   (%d) (%d) (%d) (%d) (%d) (%d)\n"
      "    a   b   c   d   e   f\n\n"
      "          %s (P2)\n\n"
      "Captures:\n"
      "  %s: %d\n"
      "  %s: %d\n"
      "================================\n",
      player1.name,
      board->board[0], board->board[1], board->board[2],
      board->board[3], board->board[4], board->board[5],
      board->board[6], board->board[7], board->board[8],
      board->board[9], board->board[10], board->board[11],
      player2.name,
      player1.name, board->player1_captures,
      player2.name, board->player2_captures
   );

}

static void list_commands(Client* client, char* response) {
   strncat(response, "Here is the list of all commands: \n", BUF_SIZE - strlen(response) - 1);
   strncat(response, "- /players : to list every usernames of players connected.\n", BUF_SIZE - strlen(response) - 1);
   strncat(response, "- /matches : to list every ongoing matches.\n", BUF_SIZE - strlen(response) - 1);
   strncat(response, "- /challenge [username] : to challenge a player.\n", BUF_SIZE - strlen(response) - 1);
   strncat(response, "- /t [username] [message] : to chat with a player. Use 'all' to talk to every players.\n", BUF_SIZE - strlen(response) - 1);
   strncat(response, "- /bio [message] : to modify your bio.\n", BUF_SIZE - strlen(response) - 1);
   strncat(response, "- /whois [username] : to consult player's bio.\n", BUF_SIZE - strlen(response) - 1);
   strncat(response, "- /accept : to accept a challenge.\n", BUF_SIZE - strlen(response) - 1);
   strncat(response, "- /refuse : to refuse a challenge.\n", BUF_SIZE - strlen(response) - 1);
}

static void talk_to(Client* clients, Client* sender, int actual, char* buffer, char* response) {
   char username[BUF_SIZE];
   char message[BUF_SIZE];
   parse_command(buffer, username, message, 1, 1);
   int found = 0; 

   if (!strcmp(username, "all")) {
      send_message_to_all_clients(clients, *sender, actual, message, 0);
      found = 1;
   } else {
      for (int i = 0; i<actual; i++) {
         if (!strcmp(clients[i].name, username)) {
            const char buffer[BUF_SIZE];
            snprintf(buffer, BUF_SIZE - strlen(buffer) - 1, "%s whispered to you : %s", sender->name, message);
            write_client(clients[i].sock, buffer);
            found = 1;
            break;
         }
      }
   }
   if (found) {   
      snprintf(response, BUF_SIZE - strlen(response) - 1, "You've send to %s : %s", username, message );
   } else {
      snprintf(response, BUF_SIZE - strlen(response) - 1, "[ERROR] User not found." );
   }
}

static void accept_challenge(Game** games, Client* clientList, Client* challengee, char* response, int actual) {

   Client *challenger = challengee->challenger;

   snprintf(response, BUF_SIZE - sizeof(response),"You've accepted %s's challenge.", challenger->name);
   write_client(challengee->sock, response);

   snprintf(response, BUF_SIZE - sizeof(response), "%s accepted your challenge!", challengee->name);
   write_client(challenger->sock, response);
   challenger->status = IN_GAME;
   challengee->status = IN_GAME;
   
   Board* board = create_board();
   if (!board) {
      snprintf(response, BUF_SIZE, "[ERROR] Failed to create game board.");
      write_client(challenger->sock, response);
      write_client(challengee->sock, response);
      challenger->status = AVAILABLE;
      challengee->status = AVAILABLE;
      return;   // Exit if board creation failed
   }

   // Créer la partie
   Game* new_game = malloc(sizeof(Game));
   if (!new_game) {
      free(board);
      challenger->status = AVAILABLE;
      challengee->status = AVAILABLE;
      return;
   }
   
   if (create_game(challenger, challengee, games, board, new_game) != 0) {
      free(board);
      free(new_game);
      challenger->status = AVAILABLE;
      challengee->status = AVAILABLE;
      return;
   }
   
   // Créer une structure pour passer les arguments au thread
   Context* args = malloc(sizeof(Context));
   args->clients = clientList;
   args->current_game = new_game;
   args->actual = actual;
   args->games = games;

   pthread_t game_thread;
   if (pthread_create(&game_thread, NULL, play_thread, args) != 0) {
      perror("pthread_create");
      free(args);
      challenger->status = AVAILABLE;
      challengee->status = AVAILABLE;
      save_game(&challenger, &challengee, board, new_game);
      return;
   }
   
   pthread_detach(game_thread); // Le thread se nettoie automatiquement
}


// Wrapper pour play() compatible avec pthread
void* play_thread(void* arg) {
   Context* args = (Context*)arg;
   play(args->games, args->clients, 
        args->current_game->player1, args->actual, 
        args->current_game->player2, args->current_game);
   // Remettre les joueurs disponibles
   for (int i = 0; i < args->actual; i++) {
      if (args->clients[i].sock == args->current_game->player1.sock ||
          args->clients[i].sock == args->current_game->player2.sock) {
         args->clients[i].status = AVAILABLE;
      }
   }
   remove_game(args->games, args->current_game);
   
   free(args);
   return NULL;
}

static void refuse_challenge(Client* challengee, char* response) {
   Client *challenger = challengee->challenger;
   if (!challenger) return; 
   challengee->challenger = NULL;
   challengee->status = AVAILABLE;
   challenger->status = AVAILABLE;

   snprintf(response, BUF_SIZE, "%s declined your challenge.", challengee->name);
   write_client(challenger->sock, response);

   snprintf(response, BUF_SIZE, "You've declined %s's challenge.", challenger->name);
}

static void observe_game(Game** games, char* buffer, Client* client, char* response) {
   int index_game = 0;
   if (client->status != AVAILABLE) {
      snprintf(response, BUF_SIZE, "You must be available to observe a challenge.");
      return;
   }
   Game* game_observed;
   if (games[index_game] != NULL) {
      game_observed = games[index_game];
      game_observed->viewers[game_observed->nb_viewers++] = client; 
   }
   snprintf(response, BUF_SIZE, "You're now observing %s challenge.", game_observed->game_name);
}

int play(Game** games, Client* clients, Client player1, int actual, Client player2, Game* game) {
   char buffer[BUF_SIZE];

   snprintf(buffer, sizeof(buffer), "Game launching...!\n");
   write_client(player1.sock, buffer);
   write_client(player2.sock, buffer);
   printf("Game launching...\n");

   // Create the board
   Board* board = game->board;

   int clockwise = rand() % 2;
   choose_clockwise(board, clockwise);
   char* orientation = clockwise ? "clockwise" : "counterclockwise";

   // Game loop
   while (!game_over(board)) {
      int num_player = board->round % 2;  // Determine which player's turn
      Client* actual_player = (num_player == 0) ? &player1 : &player2;
      Client* opponent = (num_player == 0) ? &player2 : &player1;
      int place;
      char place_char;

      // Print the board and ask for a move
      print_board(board, buffer, player1, player2);
      write_client(player1.sock, buffer);
      write_client(player2.sock, buffer);
      for (int i=0; i<game->nb_viewers; i++) {
         Client* client = game->viewers[i];
         write_client(client->sock, buffer);
      }

      snprintf(buffer, sizeof(buffer), ">>> [Round %d] It's %s turn ! (%s) <<<\n", board->round+1, actual_player->name, orientation); 
      write_client(player1.sock, buffer);
      write_client(player2.sock, buffer);
      for (int i=0; i<game->nb_viewers; i++) {
         Client* client = game->viewers[i];
         write_client(client->sock, buffer);
      }

      snprintf(buffer, sizeof(buffer), "\nEnter your move"); 
      write_client(actual_player->sock, buffer);

      int valid_move = 0 ;
      while (!valid_move) {
         // D'abord checker si l'adversaire a envoyé des commandes
         int has_opponent_msg;
         Message opponent_msg = queue_try_pop(opponent->queue, &has_opponent_msg);
         
         if (has_opponent_msg) {
            if (opponent_msg.content[0] == '/') {
               // Commande de l'adversaire - traiter immédiatement
               printf("Commande de l'adversaire %s : %s\n", opponent->name, opponent_msg.content);
               treat_command(games, clients, opponent, actual, opponent_msg.content, 1);   
            }
            // Si ce n'est pas une commande, on ignore (l'adversaire ne peut pas jouer)
         }
         
         // Maintenant checker le joueur actuel (sans bloquer)
         int has_player_msg;
         Message player_msg = queue_try_pop(actual_player->queue, &has_player_msg);
         
         if (has_player_msg) {
            if (player_msg.content[0] == '/') {
               // Commande du joueur actuel
               printf("Commande du joueur actuel %s : %s\n", actual_player->name, player_msg.content);
               treat_command(games, clients, actual_player, actual, player_msg.content, 1);
               
               // Redemander un coup
               snprintf(buffer, sizeof(buffer), "\nEnter your move"); 
               write_client(actual_player->sock, buffer);
               
            } else if (strlen(player_msg.content) == 1) {
               // C'est un coup !
               place_char = player_msg.content[0];
               place = letter_to_int(place_char);
               valid_move = 1;
               
            } else {
               // Message invalide
               snprintf(buffer, sizeof(buffer), "Invalid input. Enter a letter (a-f) or a command (/help)");
               write_client(actual_player->sock, buffer);
            }
         }
         
         if (!valid_move) {
            usleep(10000); // 10ms
         }
      }

      // Execute the player's turn
      int turn = player_turn(board, place);
      switch(turn) {
         case -1: snprintf(buffer, BUF_SIZE, "You cannot play this move."); break;
         case -2: snprintf(buffer, BUF_SIZE, "Invalid position!"); break;
         case -3: snprintf(buffer, BUF_SIZE, "Empty cell!"); break;
         case -4: snprintf(buffer, BUF_SIZE, "Not valid for clockwise."); break;
         default: turn = 0; break;
      }
      if(turn < 0) {
         write_client(actual_player->sock, buffer);
         continue; // redemander un coup
      }
      
   }

   // Game over, print the final scores

   snprintf(buffer, sizeof(buffer), "\nGame over! Final scores:\n");
   snprintf(buffer, sizeof(buffer), "Player 1 captures: %d", board->player1_captures);
   snprintf(buffer, sizeof(buffer), "Player 2 captures: %d", board->player2_captures);
   write_client(player1.sock, buffer);
   write_client(player2.sock, buffer); 
   for (int i=0; i<game->nb_viewers; i++) {
      Client* client = game->viewers[i];
      write_client(client->sock, buffer);
   }
   

   if (board->player1_captures > board->player2_captures) {
      snprintf(buffer, sizeof(buffer), "Player 1 wins!");
   } else if (board->player2_captures > board->player1_captures) {
      snprintf(buffer, sizeof(buffer), "Player 2 wins!");
   } else {
      snprintf(buffer, sizeof(buffer), "It's a tie!");
   }
   write_client(player1.sock, buffer);
   write_client(player2.sock, buffer);
   for (int i=0; i<game->nb_viewers; i++) {
      Client* client = game->viewers[i];
      write_client(client->sock, buffer);
   }

   return 0;

}

static int create_game(Client *player1, Client *player2, Game **gamelist, Board *board, Game *new_game) {
   // Generate a base game name
   char name[BUF_SIZE];
   snprintf(name, sizeof(name), "%s VS %s", player1->name, player2->name);

   // Initialize the unique name with the base name
   char unique_name[BUF_SIZE];
   strncpy(unique_name, name, sizeof(unique_name));

   // Check if any existing game in the list has the same base name (contains the base name)
   int name_exists = 0;
   int available_pos = -1;
   for (int j = 0; j < MAX_GAMES; j++) {
      if (gamelist[j] != NULL && strstr(gamelist[j]->game_name, name) != NULL) {
         name_exists += 1;
      }
      if(gamelist[MAX_GAMES - j - 1]  == NULL){
         available_pos = MAX_GAMES - j - 1;
      }
   }

   // If a game with the same base name exists, append a number to make it unique
   if (name_exists) {
      snprintf(unique_name, sizeof(unique_name), "%s (%d)", name,name_exists); 
   }

   new_game->player1 = *player1;
   new_game->player2 = *player2;
   strncpy(new_game->game_name, unique_name, sizeof(new_game->game_name));
   new_game->board = board;

   if(available_pos == -1){
      free(new_game);
      return -2;
   }
   gamelist[available_pos] = new_game;
   return 0;
}

static int save_game(Client *player1, Client *player2, Board *board, Game *game){
   game->player1 = *player1;
   game->player2 = *player2;
   game->board = board;
   return 0;
}

static int remove_game(Game **gamelist, Game *game){
   for (int j = 0; j < MAX_GAMES; j++) {
      if (gamelist[j] == game) {
         gamelist[j] = NULL;
         if(game->board)free(game->board);
         if(game->viewers)free(game->viewers);
         free(game);
         break;
      }
   }
   return 0;
}

static void remove_client(Client *clients, int to_remove, int *actual)
{
   /* we remove the client in the array */
   memmove(clients + to_remove, clients + to_remove + 1, (*actual - to_remove - 1) * sizeof(Client));
   /* number client - 1 */
   (*actual)--;
}

static void send_message_to_all_clients(Client *clients, Client sender, int actual, const char *buffer, char from_server)
{
   int i = 0;
   for(i = 0; i < actual; i++)
   {
      /* we don't send message to the sender */
      if(sender.sock != clients[i].sock)
      {
         char message[BUF_SIZE];
         message[0] = 0;
         if(from_server == 0)
         {
            strncpy(message, sender.name, BUF_SIZE - 1);
            strncat(message, " : ", sizeof message - strlen(message) - 1);
         }
         strncat(message, buffer, sizeof message - strlen(message) - 1);
         write_client(clients[i].sock, message);
      }
   }
}

static int init_connection(void)
{
   SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
   SOCKADDR_IN sin = { 0 };
   int optval = 1; // pour activer SO_REUSEADDR

   if(sock == INVALID_SOCKET)
   {
      perror("socket()");
      exit(errno);
   }

   if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0)
   {
      perror("setsockopt(SO_REUSEADDR)");
      exit(errno);
   }

   sin.sin_addr.s_addr = htonl(INADDR_ANY);
   sin.sin_port = htons(PORT);
   sin.sin_family = AF_INET;

   if(bind(sock,(SOCKADDR *) &sin, sizeof sin) == SOCKET_ERROR)
   {
      perror("bind()");
      exit(errno);
   }

   if(listen(sock, MAX_CLIENTS) == SOCKET_ERROR)
   {
      perror("listen()");
      exit(errno);
   }
   printf("Server waiting for players...\n");
   return sock;
}

static void end_connection(int sock)
{
   closesocket(sock);
}

static int read_client(SOCKET sock, char *buffer)
{
   int n = 0;

   if((n = recv(sock, buffer, BUF_SIZE - 1, 0)) < 0)
   {
      perror("recv()");
      /* if recv error we disonnect the client */
      n = 0;
   }

   buffer[n] = 0;

   return n;
}

static void write_client(SOCKET sock, const char *buffer)
{
   if(send(sock, buffer, strlen(buffer), 0) < 0)
   {
      perror("send()");
      exit(errno);
   }
}

static void parse_command(const char *buffer, char *username, char *message, int username_bool, int message_bool)
{
   const char *p = strchr(buffer, ' '); // première espace
   if (!p) return; // aucune donnée après la commande

   p++; // sauter l'espace
   
   const char *q = strchr(p, ' '); // deuxième espace

   if (q && username_bool && message_bool) {
      // user + message
      size_t len = q - p;
      strncpy(username, p, len);
      username[len] = '\0';

      strncpy(message, q + 1, BUF_SIZE - 1);
      message[BUF_SIZE - 1] = '\0';
   } else if (username_bool) {
      // seulement user
      strncpy(username, p, BUF_SIZE - 1);
      username[BUF_SIZE - 1] = '\0';
      message[0] = '\0';
   } else if (message_bool) {
      // seulement message
      strncpy(message, p, BUF_SIZE - 1);
      message[BUF_SIZE - 1] = '\0';
      username[0] = '\0';
   }
}


static void treat_command(Game **games, Client *clients, Client* sender, int actual, const char *buffer, int in_game) {
   
   char response[BUF_SIZE];
   response[0] = 0;
   if (!strcmp(buffer, "/players")) {
      list_clients(clients, actual, response);  
   } else if (!strcmp(buffer, "/games")) {
      list_games(games, response);     
   } else if (!strcmp(buffer, "/help")) {
      list_commands(sender, response);
   } else if (!strncmp(buffer, "/challenge ", 11)) {
      challenge_player(clients, sender, actual, buffer);
   } else if (!strncmp(buffer, "/bio ", 5)) {
      modify_bio(sender, buffer, response); 
   } else if (!strncmp(buffer, "/whois ", 7)) {
      consult_client(clients, actual, buffer, response);
   } else if (!strncmp(buffer, "/t ", 3)) {
      talk_to(clients, sender, actual, buffer, response);
   } else if (!strcmp(buffer, "/accept")) {
      accept_challenge(games, clients, sender, response, actual);
   } else if (!strcmp(buffer, "/refuse")) {
      refuse_challenge(sender, response);
   } else if(!strncmp(buffer, "/observe ", 9)) {
      observe_game(games, buffer, sender, response);
   } else {
      strcpy(response, "Command not found. Try /help to get the commands list.");
   }

   write_client(sender->sock, response);
}

int main(int argc, char **argv)
{
   init();

   app();

   end();

   return EXIT_SUCCESS;
}
