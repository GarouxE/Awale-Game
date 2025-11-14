#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <ctype.h>
#include <time.h>

#include "server.h"
#include "../game/game.h"
#include "savegame.h"
#include "ranking.h"
#include "friends.h"

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

         /* check username uniqueness */
         if (!is_username_unique(clients, actual, buffer)) {
            write_client(csock, "[ERROR] Username already taken\n");
            closesocket(csock);
            continue;
         }

         /* what is the new maximum fd ? */
         max = csock > max ? csock : max;

         FD_SET(csock, &rdfs);

         Client c = { csock };
         strncpy(c.name, buffer, BUF_SIZE - 1);
         c.name[BUF_SIZE - 1] = '\0';
         c.status = AVAILABLE;
         c.private_mode = 0;
         c.friend_count = 0;
         for (int _fi = 0; _fi < MAX_FRIENDS; _fi++) c.friends[_fi][0] = '\0';
         /* load persisted score if any */
         c.score = load_score_for_user("scores.txt", c.name);
         
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
                  /* persist score on disconnect */
                  save_score_for_user("scores.txt", client->name, client->score);
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

/* ---------- Persistence helpers for scores (simple text file) ---------- */
static int load_score_for_user(const char *filename, const char *username) {
   FILE *f = fopen(filename, "r");
   if (!f) return 0; /* treat missing file as zero scores */
   char line[BUF_SIZE];
   char name[BUF_SIZE];
   int score;
   while (fgets(line, sizeof(line), f)) {
      if (sscanf(line, "%1023s %d", name, &score) == 2) {
         if (strcmp(name, username) == 0) {
            fclose(f);
            return score;
         }
      }
   }
   fclose(f);
   return 0;
}

static int save_score_for_user(const char *filename, const char *username, int score) {
   /* Read original and write to tmp, updating or appending the user line */
   char tmpname[256];
   snprintf(tmpname, sizeof(tmpname), "%s.tmp", filename);
   FILE *fin = fopen(filename, "r");
   FILE *fout = fopen(tmpname, "w");
   int found = 0;
   char line[BUF_SIZE];
   char name[BUF_SIZE];
   int s;

   if (!fout) return -1;

   if (fin) {
      while (fgets(line, sizeof(line), fin)) {
         if (sscanf(line, "%1023s %d", name, &s) == 2) {
            if (strcmp(name, username) == 0) {
               fprintf(fout, "%s %d\n", username, score);
               found = 1;
            } else {
               fprintf(fout, "%s %d\n", name, s);
            }
         }
      }
      fclose(fin);
   }

   if (!found) {
      fprintf(fout, "%s %d\n", username, score);
   }
   fclose(fout);
   /* replace original file */
   remove(filename);
   rename(tmpname, filename);
   return 0;
}

static int find_client_index_by_name(Client *clients, int actual, const char *name) {
   for (int i = 0; i < actual; i++) {
      if (strcmp(clients[i].name, name) == 0) return i;
   }
   return -1;
}

static int is_username_unique(Client *clients, int actual, const char *name) {
   return find_client_index_by_name(clients, actual, name) == -1;
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
         clients[i].status == IN_GAME ? " (in game)" : 
         clients[i].status == OBSERVING ? " (observing)" : " (waiting)",
         BUF_SIZE - strlen(response) - 1);
   }

}

static void list_games(Game **games, char* response){
   strncat(response, "Here is the list of ongoing matches:", BUF_SIZE - strlen(response) - 1);

   int count = 0;
   for (int i = 0; i < MAX_GAMES; i++) {
      if (games[i] != NULL) {
         char temp[128];
         snprintf(temp, sizeof(temp), "\n - %d : ", i);
         strncat(response, temp, BUF_SIZE - strlen(response) - 1);

         if (games[i]->game_name[0] == '\0') {
               strncat(response, "[Unnamed game]", BUF_SIZE - strlen(response) - 1);
         } else {
               strncat(response, games[i]->game_name, BUF_SIZE - strlen(response) - 1);
         }
         count++;
      }
   }

   if (count == 0) {
      strncat(response, "\nNo match is currently ongoing.", BUF_SIZE - strlen(response) - 1);
   }

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
   strncat(response, "- /accept : to accept a challenge.\n", BUF_SIZE - strlen(response) - 1);
   strncat(response, "- /bio [message] : to modify your bio.\n", BUF_SIZE - strlen(response) - 1);
   strncat(response, "- /challenge [username] : to challenge a player.\n", BUF_SIZE - strlen(response) - 1);
   strncat(response, "- /friend <add|remove> [username] : add or remove a friend allowed to spectate when private.\n", BUF_SIZE - strlen(response) - 1);
   strncat(response, "- /friends : list your friends.\n", BUF_SIZE - strlen(response) - 1);
   strncat(response, "- /end : Ends/forfeit an ongoing game.\n", BUF_SIZE - strlen(response) - 1);
   strncat(response, "- /games : to list every ongoing games.\n", BUF_SIZE - strlen(response) - 1);
   strncat(response, "- /observe [challenge] : to observe ongoing games.\n", BUF_SIZE - strlen(response) - 1);
   strncat(response, "- /players : to list every usernames of players connected.\n", BUF_SIZE - strlen(response) - 1);
   strncat(response, "- /private <on|off> : enable/disable private mode for your matches.\n", BUF_SIZE - strlen(response) - 1);
   strncat(response, "- /quit : to quit observer mode.\n", BUF_SIZE - strlen(response) - 1);
   strncat(response, "- /ranking <me|[number]>: see the player ranking.\n", BUF_SIZE - strlen(response) - 1);
   strncat(response, "- /refuse : to refuse a challenge.\n", BUF_SIZE - strlen(response) - 1);
   strncat(response, "- /savedgame : shows a list of saved games.\n", BUF_SIZE - strlen(response) - 1);
   strncat(response, "- /t [username] [message] : to chat with a player. Use 'all' to talk to every players.\n", BUF_SIZE - strlen(response) - 1);
   strncat(response, "- /viewgame [number] : Shows the saved game.\n", BUF_SIZE - strlen(response) - 1);
   strncat(response, "- /whois [username] : to consult player's bio.\n", BUF_SIZE - strlen(response) - 1);

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

static void observe_game(Game** games, Client* clients, int actual, char* buffer, Client* client, char* response) {
   int index_game;
   Game* game_observed;
   if (sscanf(buffer, "/observe %d", &index_game) != 1) {
      snprintf(response, BUF_SIZE, "[ERROR] Challenge not found.");
      return;
   }
   else if (client->status != AVAILABLE) {
      snprintf(response, BUF_SIZE, "You must be available to observe a challenge.");
      return;
   }
   else if (games[index_game] == NULL) {
      snprintf(response, BUF_SIZE, "[ERROR] Challenge not found.");
      return;
   }

   game_observed = games[index_game];
   int p1_idx = find_client_index_by_name(clients, actual, game_observed->player1.name);
   int p2_idx = find_client_index_by_name(clients, actual, game_observed->player2.name);
   Client *p1_live = (p1_idx != -1) ? &clients[p1_idx] : &game_observed->player1;
   Client *p2_live = (p2_idx != -1) ? &clients[p2_idx] : &game_observed->player2;

   /* Enforce privacy: if either player enabled private_mode, only allow
      observers who are in that player's friend list. */
   int allow = 1;
   /* check player1 */
   /* The game stores copies of the players (including their friend lists and
      private_mode). We'll check those copies to decide whether the observer is
      allowed to join. */
   /* If player1 is private and observer is not in player1 friends -> deny */
   if (p1_live->private_mode) {
      int found = 0;
      for (int i = 0; i < p1_live->friend_count; i++) {
         if (strcmp(p1_live->friends[i], client->name) == 0) { found = 1; break; }
      }
      if (!found) allow = 0;
   }
   if (p2_live->private_mode) {
      int found = 0;
      for (int i = 0; i < p2_live->friend_count; i++) {
         if (strcmp(p2_live->friends[i], client->name) == 0) { found = 1; break; }
      }
      if (!found) allow = 0;
   }

   if (!allow) {
      snprintf(response, BUF_SIZE, "[ERROR] This match is private. You are not allowed to observe it.");
      return;
   }

   game_observed->viewers[game_observed->nb_viewers++] = client;
   client->game_location = index_game;
   client->status = OBSERVING;
   snprintf(response, BUF_SIZE, "You're now observing %s challenge.", game_observed->game_name);
   
}

static void quit_game(Game** games, Client* sender, char* response ) {
   if (sender->status != OBSERVING) {
      snprintf(response, BUF_SIZE, "[ERROR] You're not currently observing a challenge.");
      return;
   }
   if (games[sender->game_location] == NULL) {
      snprintf(response, BUF_SIZE, "[ERROR] You're not observing this challenge.");
      return;
   }
   games[sender->game_location]->nb_viewers--;
   sender->game_location = MAX_GAMES + 1;
   sender->status = AVAILABLE;
   snprintf(response, BUF_SIZE, "You've left observer mode.");
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
   int aborted = 0; /* set to 1 when a player aborts the match with /end */
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
               /* special in-game command: /end -> abort match, award captures and save */
               if (strcmp(player_msg.content, "/end") == 0 ) {
                  int turn = player_turn(board, 18);
                  /* Apply capture-based scoring to both players (like normal end) */
                  int idx1 = find_client_index_by_name(clients, actual, player1.name);
                  if (idx1 != -1) {
                     clients[idx1].score += board->player1_captures;
                     save_score_for_user("scores.txt", clients[idx1].name, clients[idx1].score);
                  }
                  int idx2 = find_client_index_by_name(clients, actual, player2.name);
                  if (idx2 != -1) {
                     clients[idx2].score += board->player2_captures;
                     save_score_for_user("scores.txt", clients[idx2].name, clients[idx2].score);
                  }

                  /* Notify players and viewers: the other player wins by forfeit */
                  snprintf(buffer, sizeof(buffer), "%s aborted the match. %s wins by forfeit!", actual_player->name, opponent->name);
                  write_client(player1.sock, buffer);
                  write_client(player2.sock, buffer);
                  for (int v = 0; v < game->nb_viewers; v++) {
                     Client* client = game->viewers[v];
                     write_client(client->sock, buffer);
                  }

                  /* Persist the game record */
                  save_game(&player1, &player2, board, game);

                  aborted = 1;
                  break; /* exit inner input loop and then outer loop will be broken */
               }

               treat_command(games, clients, actual_player, actual, player_msg.content, 1);

               /* Redemander un coup */
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

   if (aborted) break;

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

   // Game over: the match was aborted by a player (/end) = forfeit or player lost
   
   if (!aborted) {
      /* compose final scores message */
      buffer[0] = '\0';
      snprintf(buffer + strlen(buffer), sizeof(buffer) - strlen(buffer), "\nGame over! Final scores:\n");
      snprintf(buffer + strlen(buffer), sizeof(buffer) - strlen(buffer), "Player 1 captures: %d\n", board->player1_captures);
      snprintf(buffer + strlen(buffer), sizeof(buffer) - strlen(buffer), "Player 2 captures: %d\n", board->player2_captures);
      write_client(player1.sock, buffer);
      write_client(player2.sock, buffer);
      for (int i=0; i<game->nb_viewers; i++) {
         Client* client = game->viewers[i];
         write_client(client->sock, buffer);
      }

      /* Award players points based on captures (add their capture counts to their persistent score) */
      int idx1 = find_client_index_by_name(clients, actual, player1.name);
      if (idx1 != -1) {
         clients[idx1].score += board->player1_captures;
         save_score_for_user("scores.txt", clients[idx1].name, clients[idx1].score);
      }
      int idx2 = find_client_index_by_name(clients, actual, player2.name);
      if (idx2 != -1) {
         clients[idx2].score += board->player2_captures;
         save_score_for_user("scores.txt", clients[idx2].name, clients[idx2].score);
      }

      char result[BUF_SIZE];
      if (board->player1_captures > board->player2_captures) {
         snprintf(result, sizeof(result), "Player 1 wins!");
      } else if (board->player2_captures > board->player1_captures) {
         snprintf(result, sizeof(result), "Player 2 wins!");
      } else {
         snprintf(result, sizeof(result), "It's a tie!");
      }

      /* send winner/tie message */
      write_client(player1.sock, result);
      write_client(player2.sock, result);
      for (int i=0; i<game->nb_viewers; i++) {
         Client* client = game->viewers[i];
         write_client(client->sock, result);
      }
   }

   return 0;

}

static int create_game(Client *player1, Client *player2, Game **gamelist, Board *board, Game *new_game) {
   // Generate a base game name
   char name[BUF_SIZE];
   if (rand() % 2) {
      Client *temp = player1;
      player1 = player2;
      player2 = temp;
   }
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

/* save_game implementation moved to src/server/savegame.c */

static int remove_game(Game **gamelist, Game *game){
   for (int j = 0; j < MAX_GAMES; j++) {
      if (gamelist[j] == game) {
         gamelist[j] = NULL;
         if(game->board)free(game->board);
         if(game->viewers) {
            for (int i = 0; i < game->nb_viewers; i++) if (game->viewers[i]) game->viewers[i]->status = AVAILABLE;
            /* game->viewers is an embedded fixed-size array in Game; do not free it */
         }
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

void write_client(SOCKET sock, const char *buffer)
{
   if(send(sock, buffer, strlen(buffer), 0) < 0)
   {
      perror("send()");
      exit(errno);
   }
}
static void copy_client(Client* dest, Client* src){
   /* shallow copy of client struct */
   *dest = *src;
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
   /* If the command comes from in-game message handling, 'sender' may be a
      local copy inside the game thread. Find and use the live client entry so
      changes (private/friends/etc.) apply immediately to the server state. */
   if (in_game && clients != NULL) {
      int live_idx = find_client_index_by_name(clients, actual, sender->name);
      if (live_idx != -1) {
         sender = &clients[live_idx];
      }
   }
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
      observe_game(games, clients, actual, buffer, sender, response);
   } else if (!strcmp(buffer, "/quit")) {
      quit_game(games, sender, response); 
   } else if (!strcmp(buffer, "/savedgames")) {
      list_saved_games(sender, response);
   } else if (!strncmp(buffer, "/viewgame ", 9)) {
      view_saved_game(sender, buffer, response);
   } else if (!strcmp(buffer, "/friends")) {
      list_friends(sender, response);
   } else if (!strncmp(buffer, "/friend ", 8)) {
      friend_command(sender, buffer, response);
   } else if (!strncmp(buffer, "/private", 8)) {
      set_private_mode(sender, buffer, response);
   } else if (!strncmp(buffer, "/ranking", 8)) {
      char arg[64] = "";
      if (sscanf(buffer, "/ranking %63s", arg) == 1) {
         user_ranking(clients, sender->sock, actual, sender, arg);
      } else {
         user_ranking(clients, sender->sock, actual, sender, "");
      }
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
