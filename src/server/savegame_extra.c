#include <stdio.h>
#include <string.h>
#include "savegame.h"

void list_saved_games(Client* sender, char* response) {
   response[0] = '\0';
   FILE *f = fopen("games.txt", "r");
   if (!f) {
      snprintf(response, BUF_SIZE, "[INFO] No saved games found.");
      return;
   }
   char line[BUF_SIZE];
   int idx = 0;
   char date[128] = "(unknown)";
   char gname[BUF_SIZE] = "(unnamed)";
   while (fgets(line, sizeof(line), f)) {
      if (strcmp(line, "=== GAME START ===\n") == 0) {
         /* read Date and GameName lines */
         if (fgets(line, sizeof(line), f) && sscanf(line, "Date: %127[^\n]", date)) {}
         if (fgets(line, sizeof(line), f) && sscanf(line, "GameName: %1023[^\n]", gname)) {}
         idx++;
         char entry[256];
         snprintf(entry, sizeof(entry), "%d. %s - %s\n", idx, date, gname);
         strncat(response, entry, BUF_SIZE - strlen(response) - 1);
      }
   }
   fclose(f);
   if (idx == 0) snprintf(response, BUF_SIZE, "[INFO] No saved games found.");
}

void view_saved_game(Client* sender, const char* buffer, char* response) {
   response[0] = '\0';
   int n = 0;
   if (sscanf(buffer, "/viewgame %d", &n) != 1 || n <= 0) {
      snprintf(response, BUF_SIZE, "[ERROR] Usage: /viewgame N (N positive).");
      return;
   }
   FILE *f = fopen("games.txt", "r");
   if (!f) { snprintf(response, BUF_SIZE, "[INFO] No saved games found."); return; }
   char line[BUF_SIZE];
   int idx = 0;
   int in_block = 0;
   char block[BUF_SIZE]; block[0] = '\0';
   while (fgets(line, sizeof(line), f)) {
      if (strcmp(line, "=== GAME START ===\n") == 0) {
         in_block = 1;
         idx++;
         /* start new block capture */
         block[0] = '\0';
         strncat(block, line, BUF_SIZE - strlen(block) - 1);
         continue;
      }
      if (in_block) {
         strncat(block, line, BUF_SIZE - strlen(block) - 1);
         if (strcmp(line, "=== GAME END ===\n") == 0) {
            in_block = 0;
            if (idx == n) {
               strncpy(response, block, BUF_SIZE - 1);
               response[BUF_SIZE - 1] = '\0';
               fclose(f);
               return;
            }
         }
      }
   }
   fclose(f);
   snprintf(response, BUF_SIZE, "[ERROR] Saved game %d not found.", n);
}
