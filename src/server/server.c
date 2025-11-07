#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "server.h"
#include "client.h"
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
                     treat_command(clients, client, actual, buffer);   
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

static int challenge_player(Client *clientList, Client client, int actual, char *buffer) {
   const char *username = buffer + 11; 
   char message[BUF_SIZE];
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
               "ERROR: no user found with the username '%s'", username);
      write_client(client.sock, message);
      return -1;
   }

   snprintf(message, sizeof(message),"You've challenged %s !", challengee->name);
   write_client(client.sock, message);

   // Ask the target player if they accept the challenge
   snprintf(message, sizeof(message),"%s has challenged you! Do you accept? (y/n)", client.name);
   write_client(challengee->sock, message);

   // Read response
   memset(buffer, 0, sizeof(buffer));
   read_client(challengee->sock, buffer);

   // Handle decline
   if (buffer[0] == 'n' || buffer[0] == 'N') {
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
      play(client, *challengee);
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
   const char *new_bio = buffer + 5;
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
   const char *username = buffer + 7;
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
      strcpy(response, "User not found.");
   }
}

static void print_board(Board* board, char* buffer) {
   int offset = 0;
   offset += snprintf(buffer + offset, BUF_SIZE - offset, "#====================================#\n");
   offset += snprintf(buffer + offset, BUF_SIZE - offset, "        Player1\n\n");

   // Top row
   for (int i = 0; i < 6; i++) {
      offset += snprintf(buffer + offset, BUF_SIZE - offset, "%c:%d  ", 65 + i, board->board[i]);
   }
   offset += snprintf(buffer + offset, BUF_SIZE - offset, "\n");

   // Bottom row
   for (int i = 6; i < 12; i++) {
      offset += snprintf(buffer + offset, BUF_SIZE - offset, "%c:%d  ", 97 + (i-6), board->board[i]);
   }
   offset += snprintf(buffer + offset, BUF_SIZE - offset, "\n        Player2\n\n");

   // Captures
   offset += snprintf(buffer + offset, BUF_SIZE - offset, "\nPlayer 1 Captures: %d\n", board->player1_captures);
   offset += snprintf(buffer + offset, BUF_SIZE - offset, "Player 2 Captures: %d\n", board->player2_captures);
   offset += snprintf(buffer + offset, BUF_SIZE - offset, "#====================================#\n");
}


int play(Client player1, Client player2) {
   char buffer[BUF_SIZE];

   snprintf(buffer, sizeof(buffer), "Game launching...!\n");
   write_client(player1.sock, buffer);
   write_client(player2.sock, buffer);
   printf("Game launching...\n");

   // Create the board
   Board* board = create_board();
   if (!board) {
      return -1;  // Exit if board creation failed
   }
   int clockwise;
   /* printf("Enter direction (0 for counterclockwise, 1 for clockwise): ");
   scanf("%d", &clockwise); */
   choose_clockwise(board,1);


   // Game loop
   while (!game_over(board)) {
      int num_player = board->round % 2;  // Determine which player's turn
      Client actual_player = (num_player == 0) ? player1 : player2;
      int place;
      char place_char;

      // Print the board and ask for a move
      print_board(board, buffer);
      write_client(player1.sock, buffer);
      write_client(player2.sock, buffer);

      snprintf(buffer, sizeof(buffer), ">>> [Round %d] It's %s turn ! <<<", board->round, actual_player.name); 
      write_client(player1.sock, buffer);
      write_client(player2.sock, buffer);

      snprintf(buffer, sizeof(buffer), "\nEnter your move"); 
      write_client(actual_player.sock, buffer);
      memset(buffer, 0, sizeof(buffer));
      read_client(actual_player.sock, buffer);
      
      place_char = buffer[0];
      printf("place_char : %c", place_char);
      
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
      write_client(actual_player.sock, buffer);
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

static void treat_command(Client *clients, Client sender, int actual, const char *buffer) {
   
   char response[BUF_SIZE];
   response[0] = 0;
   if (!strcmp(buffer, "/list")) {
      list_clients(clients, sender, actual, response);      
   } else if (!strcmp(buffer, "/help")) {
      strcpy(response, "HELP");
   } else if (!strncmp(buffer, "/challenge ", 11)) {
      challenge_player(clients, sender, actual, buffer);
      //strcpy(response, "CHALLENGE");
   } else if (!strncmp(buffer, "/bio ", 5)) {
      modify_bio(clients, sender, actual, buffer, response); 
   } else if (!strncmp(buffer, "/whois ", 7)) {
      consult_client(clients, sender, actual, buffer, response);
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
