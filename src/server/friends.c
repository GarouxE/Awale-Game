#include <stdio.h>
#include <string.h>
#include "friends.h"

void list_friends(Client* sender, char* response) {
   response[0] = '\0';
   if (sender->friend_count == 0) {
      snprintf(response, BUF_SIZE, "[INFO] You have no friends set.");
      return;
   }
   strncat(response, "Your friends:", BUF_SIZE - strlen(response) - 1);
   for (int i = 0; i < sender->friend_count; i++) {
      strncat(response, "\n - ", BUF_SIZE - strlen(response) - 1);
      strncat(response, sender->friends[i], BUF_SIZE - strlen(response) - 1);
   }
}

void friend_command(Client* sender, const char* buffer, char* response) {
   char op[16];
   char username[BUF_SIZE];
   if (sscanf(buffer, "/friend %15s %1023s", op, username) != 2) {
      snprintf(response, BUF_SIZE, "[ERROR] Usage: /friend add|remove <username>");
      return;
   }
   if (!strcmp(op, "add")) {
      if (sender->friend_count >= MAX_FRIENDS) {
         snprintf(response, BUF_SIZE, "[ERROR] Friend list full (max %d)", MAX_FRIENDS);
         return;
      }
      for (int i = 0; i < sender->friend_count; i++) if (!strcmp(sender->friends[i], username)) {
         snprintf(response, BUF_SIZE, "[INFO] %s is already in your friend list.", username);
         return;
      }
      strncpy(sender->friends[sender->friend_count++], username, BUF_SIZE - 1);
      sender->friends[sender->friend_count - 1][BUF_SIZE - 1] = '\0';
      snprintf(response, BUF_SIZE, "[SUCCESS] %s added to your friend list.", username);
   } else if (!strcmp(op, "remove")) {
      int found = -1;
      for (int i = 0; i < sender->friend_count; i++) if (!strcmp(sender->friends[i], username)) { found = i; break; }
      if (found == -1) { snprintf(response, BUF_SIZE, "[ERROR] %s not found in your friend list.", username); return; }
      for (int j = found; j < sender->friend_count - 1; j++) strncpy(sender->friends[j], sender->friends[j+1], BUF_SIZE);
      sender->friend_count--;
      snprintf(response, BUF_SIZE, "[SUCCESS] %s removed from your friend list.", username);
   } else {
      snprintf(response, BUF_SIZE, "[ERROR] Unknown op '%s'. Use add or remove.", op);
   }
}

void set_private_mode(Client* sender, const char* buffer, char* response) {
   char arg[16];
   if (sscanf(buffer, "/private %15s", arg) != 1) {
      snprintf(response, BUF_SIZE, "[ERROR] Usage: /private on|off");
      return;
   }
   if (!strcmp(arg, "on")) {
      sender->private_mode = 1;
      snprintf(response, BUF_SIZE, "[SUCCESS] Private mode enabled. Only friends can spectate your matches.");
   } else if (!strcmp(arg, "off")) {
      sender->private_mode = 0;
      snprintf(response, BUF_SIZE, "[SUCCESS] Private mode disabled. Anyone can spectate your matches.");
   } else {
      snprintf(response, BUF_SIZE, "[ERROR] Usage: /private on|off");
   }
}
