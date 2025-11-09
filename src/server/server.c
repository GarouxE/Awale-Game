#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "server.h"
#include "client.h"
#include "../game/game.h"
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

    /* Arrays to hold game pointers for public and private games */
   Game *public_games[MAX_SAVED_GAMES];
   Game *private_games[MAX_SAVED_GAMES];

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
               Client client = clients[i];
               int c = read_client(clients[i].sock, buffer);
               /* client disconnected */
               if(c == 0)
               {
                  closesocket(clients[i].sock);
                  remove_client(clients, i, &actual);
                  strncpy(buffer, client.name, BUF_SIZE - 1);
                  strncat(buffer, " has left.", BUF_SIZE - strlen(buffer) - 1);
                  printf("%s has left.\n", client.name);
                  send_message_to_all_clients(clients, client, actual, buffer, 1);
               }
               else {
                  // Commande spéciale
                  if (buffer[0] == '/') {
                     printf("Commande reçue de %s : %s\n", client.name, buffer);
                     treat_command(clients, client, actual, buffer, 0, public_games, private_games);  
                  }
                  else send_message_to_all_clients(clients, client, actual, buffer, 0);
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

static int challenge_player(Client *clientList, Client client, int actual, char *buffer, Game **public_games, Game **private_games) {
   char username[BUF_SIZE]; 
   char message[BUF_SIZE];
   parse_command(buffer, username, message, 1, 0);
   buffer[0] = '\0';

   // Check if the challenged user exists
   Client *challengee = NULL;
   for (int i = 0; i < actual; i++) {
      if (strcmp(clientList[i].name, username) == 0) {
         challengee = &clientList[i];
         break;
      }
   }

   if (challengee == NULL) {
      snprintf(message, sizeof(message),
               "ERROR: user not found");
      write_client(client.sock, message);
      return -1;
   }

   if(challengee->sock == client.sock){
      snprintf(message, sizeof(message),"You'can't challenge yourself %s !", challengee->name);
      write_client(client.sock, message);
      return -1;
   }

   snprintf(message, sizeof(message),"You've challenged %s !", challengee->name);
   write_client(client.sock, message);


   while (1) {
      // Ask the target player if they accept the challenge
      snprintf(message, sizeof(message),"%s has challenged you! Do you accept? (y/n)", client.name);
      write_client(challengee->sock, message);

      // Read response
      memset(buffer, 0, sizeof(buffer));
      read_client(challengee->sock, buffer);
      if (buffer[0] == '0') {
         break;
      }else if (buffer[0] == '/') {
         treat_command(clientList, *challengee, actual, buffer, 1, public_games, private_games);
         continue; // On redemande un coup
      }
      else {
         snprintf(buffer, sizeof(buffer), "Invalid input, please enter 0 for counterclockwise or 1 for clockwise.\n");
         write_client(challengee->sock, buffer);
      }
   }

   // Handle decline
   if (buffer[0] != 'y' || buffer[0] == 'Y') {
      snprintf(message, sizeof(message),"You've declined %s's challenge.", client.name);
      write_client(challengee->sock, message);

      snprintf(message, sizeof(message),"%s declined your challenge.", challengee->name);
      write_client(client.sock, message);
      return -1;
   }

   // Handle accept
   snprintf(message, sizeof(message),"You've accepted %s's challenge.", client.name);
   write_client(challengee->sock, message);


   snprintf(message, sizeof(message), "%s accepted your challenge!", challengee->name);
   write_client(client.sock, message);
   int pid = fork();
   if(pid == 0) {
      play(clientList, client, actual, *challengee, public_games, private_games, type);
      exit(0);
   } 

   return 0;
}



static void list_clients(Client *clients, Client sender, int actual, char* response){
   int i = 0;
   strncat(response, "Here is the list of all users: ", BUF_SIZE - strlen(response) - 1);
   for (i =0; i<actual;i++){
      strncat(response, "\n - ", BUF_SIZE - strlen(response) - 1);
      strncat(response, clients[i].name, BUF_SIZE - strlen(response) - 1);
   }

}

static void modify_bio(Client *clients, Client sender, int actual, char* buffer, char* response) {
   char username[0];
   char new_bio[BUF_SIZE];
   parse_command(buffer, username, new_bio, 0, 1);

   if (!strcmp(new_bio,"")) {
      snprintf(response, BUF_SIZE - strlen(response) - 1,
               "ERROR: new bio should not be empty ");
      return;
   }

   for (int i = 0; i < actual; i++) {
      if (sender.sock == clients[i].sock) {
         strncpy(clients[i].bio, new_bio, BUF_SIZE - 1);
         clients[i].bio[BUF_SIZE - 1] = '\0';
         strcpy(response, "Votre bio a été mise à jour.");
         break;
      }
   }
   
}

static void consult_client(Client *clients, Client sender, int actual, char*buffer, char* response) {
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
      strcpy(response, "ERROR : User not found.");
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

static void list_commands(Client client, char* response) {
   strncat(response, "Here is the list of all commands: \n", BUF_SIZE - strlen(response) - 1);
   strncat(response, "- /list : to list every usernames of players connected.\n", BUF_SIZE - strlen(response) - 1);
   strncat(response, "- /challenge [username] : to challenge a player.\n", BUF_SIZE - strlen(response) - 1);
   //strncat(response, "- /accept", BUF_SIZE - strlen(response) - 1);
   //strncat(response, "- /refuse", BUF_SIZE - strlen(response) - 1);
   strncat(response, "- /t [username] [message] : to chat with a player. Use 'all' to talk to every players.\n", BUF_SIZE - strlen(response) - 1);
   strncat(response, "- /bio [message] : to modify your bio.\n", BUF_SIZE - strlen(response) - 1);
   strncat(response, "- /whois [username] : to consult player's bio.\n", BUF_SIZE - strlen(response) - 1);
}

static void talk_to(Client* clients, Client sender, int actual, char* buffer, char* response) {
   char username[BUF_SIZE];
   char message[BUF_SIZE];
   parse_command(buffer, username, message, 1, 1);
   int found = 0; 

   if (!strcmp(username, "all")) {
      send_message_to_all_clients(clients, sender, actual, message, 0);
      found = 1;
   } else {
      for (int i = 0; i<actual; i++) {
         if (!strcmp(clients[i].name, username)) {
            const char buffer[BUF_SIZE];
            snprintf(buffer, BUF_SIZE - strlen(buffer) - 1, "%s whispered to you : %s", sender.name, message);
            write_client(clients[i].sock, buffer);
            found = 1;
            break;
         }
      }
   }
   if (found) {   
      snprintf(response, BUF_SIZE - strlen(response) - 1, "You've send to %s : %s", username, message );
   } else {
      snprintf(response, BUF_SIZE - strlen(response) - 1, "ERROR : User not found." );
   }
}

int play(Client* clients, Client player1, int actual, Client player2, Game **public_games, Game **private_games, int type){
   char buffer[BUF_SIZE];
   //selecting who starts randomly
   if(rand() % 2){
      Client temp = player1;
      player1 = player2;
      player2 = temp;
   }


   snprintf(buffer, sizeof(buffer), "Game launching...!\n");
   write_client(player1.sock, buffer);
   write_client(player2.sock, buffer);
   printf("Game launching...\n");

   // Create the board
   Board* board = create_board();
   if (!board) {
      return -1;  // Exit if board creation failed
   }
   int clockwise = -1;

   /* printf("Enter direction (0 for counterclockwise, 1 for clockwise): ");
   scanf("%d", &clockwise); */
   while (1) {
      snprintf(buffer, sizeof(buffer), "%s is choosing a game direction: \n", player1.name);
      write_client(player1.sock, buffer);
      snprintf(buffer, sizeof(buffer), "Enter direction (0 for counterclockwise, 1 for clockwise): \n");
      write_client(player1.sock, buffer);
      read_client(player1.sock, buffer);
      if (buffer[0] == '0') {
         clockwise = 0;
         break;
      } else if (buffer[0] == '1') {
         clockwise = 1;
         break;
      } else if (buffer[0] == '/') {
         treat_command(clients, player1, actual, buffer, 1, public_games, private_games);
         continue; // On redemande un coup
      }
      else {
         snprintf(buffer, sizeof(buffer), "Invalid input, please enter 0 for counterclockwise or 1 for clockwise.\n");
         write_client(player1.sock, buffer);
      }
   }
   choose_clockwise(board, clockwise);
   char* orientation = clockwise ? "clockwise" : "counterclockwise";

   // Game loop
   while (!game_over(board)) {
      int num_player = board->round % 2;  // Determine which player's turn
      Client actual_player = (num_player == 0) ? player1 : player2;
      int place;
      char place_char = '/';

      // Print the board and ask for a move
      print_board(board, buffer, player1, player2);
      write_client(player1.sock, buffer);
      write_client(player2.sock, buffer);

      snprintf(buffer, sizeof(buffer), ">>> [Round %d] It's %s turn ! (%s) <<<", board->round, actual_player.name, orientation); 
      write_client(player1.sock, buffer);
      write_client(player2.sock, buffer);

      while (place_char == '/') {
         snprintf(buffer, sizeof(buffer), "\nEnter your move"); 
         write_client(actual_player.sock, buffer);

         memset(buffer, 0, sizeof(buffer));
         read_client(actual_player.sock, buffer);

         if (buffer[0] == '/') {
            treat_command(clients, actual_player, actual, buffer, 1, public_games, private_games);
            continue; // On redemande un coup
         }

         place_char = buffer[0];
      }

      place = letter_to_int(place_char);  // Convert the letter to an index

      // Execute the player's turn
      int turn = player_turn(board, place);
      if (turn == -1) {
         snprintf(buffer, sizeof(buffer), "You cannot play this move.");
         continue;
      }
      if(turn == -2) snprintf(buffer, sizeof(buffer), "Invalid position! Please choose a valid position.\n");
      if(turn == -3) snprintf(buffer, sizeof(buffer), "Empty cell! Please choose a valid position.\n");
      if(turn == -4) snprintf(buffer, sizeof(buffer), "Not a valid entry for the clockwise value! Please choose a valid position.\n");
      if (turn < 0) write_client(actual_player.sock, buffer);
   }

   // Game over, print the final scores

   snprintf(buffer, sizeof(buffer), "\nGame over! Final scores:\n");
   snprintf(buffer, sizeof(buffer), "Player 1 captures: %d", board->player1_captures);
   snprintf(buffer, sizeof(buffer), "Player 2 captures: %d", board->player2_captures);
   write_client(player1.sock, buffer);
   write_client(player2.sock, buffer);
   

   if (board->player1_captures > board->player2_captures) {
      snprintf(buffer, sizeof(buffer), "Player 1 wins!");
   } else if (board->player2_captures > board->player1_captures) {
      snprintf(buffer, sizeof(buffer), "Player 2 wins!");
   } else {
      snprintf(buffer, sizeof(buffer), "It's a tie!");
   }
   write_client(player1.sock, buffer);
   write_client(player2.sock, buffer);
   // Don't forget to free the dynamically allocated memory for the board
   free(board);

   return 0;

   }


static void remove_client(Client *clients, int to_remove, int *actual){
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


static void treat_command(Client *clients, Client sender, int actual, const char *buffer, int in_game, Game **public_games, Game **private_games){
   
   char response[BUF_SIZE];
   response[0] = 0;
   if (!strcmp(buffer, "/list")) {
      list_clients(clients, sender, actual, response);      
   } else if (!strcmp(buffer, "/help")) {
      list_commands(sender, response);
   } else if (!strncmp(buffer, "/challenge ", 11)) {
      challenge_player(clients, sender, actual, buffer, public_games, private_games);
   } else if (!strncmp(buffer, "/bio ", 5)) {
      modify_bio(clients, sender, actual, buffer, response); 
   } else if (!strncmp(buffer, "/whois ", 7)) {
      consult_client(clients, sender, actual, buffer, response);
   } else if (!strncmp(buffer, "/t ", 3)) {
      talk_to(clients, sender, actual, buffer, response);
   } else {
      strcpy(response, "Command not found. Try /help to get the commands list.");
   }

   write_client(sender.sock, response);
}

int main(int argc, char **argv)
{
   init();

   app();

   end();

   return EXIT_SUCCESS;
}
