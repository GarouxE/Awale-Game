#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "ranking.h"

void user_ranking( Client *clients, SOCKET sock, int actual, Client *sender, const char *arg){
   typedef struct { char name[BUF_SIZE]; int score; } Entry;
   Entry *arr = NULL;
   int arr_n = 0;

   FILE *f = fopen("scores.txt", "r");
   if (f) {
      char line[BUF_SIZE];
      char name[BUF_SIZE];
      int s;
      while (fgets(line, sizeof(line), f)) {
         if (sscanf(line, "%1023s %d", name, &s) == 2) {
            Entry e;
            strncpy(e.name, name, BUF_SIZE - 1);
            e.name[BUF_SIZE - 1] = '\0';
            e.score = s;
            Entry *tmp = realloc(arr, sizeof(Entry) * (arr_n + 1));
            if (!tmp) { free(arr); fclose(f); write_client(sock, "[ERROR] Cannot allocate memory for ranking."); return; }
            arr = tmp;
            arr[arr_n++] = e;
         }
      }
      fclose(f);
   }

   for (int i = 0; i < actual; i++) {
      int found = -1;
      for (int j = 0; j < arr_n; j++) {
         if (strcmp(arr[j].name, clients[i].name) == 0) { found = j; break; }
      }
      if (found != -1) {
         arr[found].score = clients[i].score;
      } else {
         Entry e;
         strncpy(e.name, clients[i].name, BUF_SIZE - 1);
         e.name[BUF_SIZE - 1] = '\0';
         e.score = clients[i].score;
         Entry *tmp = realloc(arr, sizeof(Entry) * (arr_n + 1));
         if (!tmp) { free(arr); write_client(sock, "[ERROR] Cannot allocate memory for ranking."); return; }
         arr = tmp;
         arr[arr_n++] = e;
      }
   }

   if (arr_n == 0) {
      write_client(sock, "[INFO] No players with scores available.");
      free(arr);
      return;
   }

   for (int i = 0; i < arr_n - 1; i++) {
      for (int j = 0; j < arr_n - i - 1; j++) {
         if (arr[j].score < arr[j+1].score) {
            Entry tmp = arr[j]; arr[j] = arr[j+1]; arr[j+1] = tmp;
         }
      }
   }

   char message[BUF_SIZE]; message[0] = '\0';

   if (arg == NULL || arg[0] == '\0') {
      for (int i = 0; i < arr_n; i++) {
         char line[128];
         snprintf(line, sizeof(line), "%d. %s : %d\n", i + 1, arr[i].name, arr[i].score);
         strncat(message, line, BUF_SIZE - strlen(message) - 1);
      }
   } else if (!strcmp(arg, "me")) {
      if (!sender) { snprintf(message, sizeof(message), "[ERROR] Sender unknown."); }
      else {
         int pos = -1;
         for (int i = 0; i < arr_n; i++) if (!strcmp(arr[i].name, sender->name)) { pos = i; break; }
         if (pos == -1) snprintf(message, sizeof(message), "[INFO] Could not find your ranking (maybe not present in scores.txt yet)");
         else snprintf(message, sizeof(message), "%d. %s : %d\n", pos + 1, arr[pos].name, arr[pos].score);
      }
   } else {
      int n = 0; int ok = 1;
      for (size_t k = 0; k < strlen(arg); k++) if (!isdigit((unsigned char)arg[k])) ok = 0;
      if (ok) n = atoi(arg);
      if (!ok || n <= 0) {
         snprintf(message, sizeof(message), "[ERROR] Invalid argument for /ranking. Use '/ranking', '/ranking N' or '/ranking me'.");
      } else {
         int limit = n < arr_n ? n : arr_n;
         for (int i = 0; i < limit; i++) {
            char line[128];
            snprintf(line, sizeof(line), "%d. %s : %d\n", i + 1, arr[i].name, arr[i].score);
            strncat(message, line, BUF_SIZE - strlen(message) - 1);
         }
      }
   }

   write_client(sock, message);
   free(arr);
}
