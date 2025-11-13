#ifndef FRIENDS_H
#define FRIENDS_H

#include "server.h"

void list_friends(Client* sender, char* response);
void friend_command(Client* sender, const char* buffer, char* response);
void set_private_mode(Client* sender, const char* buffer, char* response);

#endif /* FRIENDS_H */
