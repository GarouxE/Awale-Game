#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "server.h"
#include "client.h"

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

   snprintf(message, sizeof(message),"You've challenged %s !", client.name);
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
   }
   else {
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
