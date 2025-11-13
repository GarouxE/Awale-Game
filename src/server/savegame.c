#include <stdio.h>
#include <time.h>
#include "savegame.h"

int save_game(Client *player1, Client *player2, Board *board, Game *game){
   /* Append a textual representation of the finished game to games.txt
      Format: a header line, date, player names, final board (12 ints), captures, optional game name
   */
   FILE *f = fopen("games.txt", "a");
   if (!f) return -1;

   time_t t = time(NULL);
   struct tm tm = *localtime(&t);

   fprintf(f, "=== GAME START ===\n");
   fprintf(f, "Date: %04d-%02d-%02d %02d:%02d:%02d\n", tm.tm_year+1900, tm.tm_mon+1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
   fprintf(f, "GameName: %s\n", game ? game->game_name : "(unnamed)");
   fprintf(f, "Player1: %s\n", player1 ? player1->name : "(unknown)");
   fprintf(f, "Player2: %s\n", player2 ? player2->name : "(unknown)");
   /* Pretty-print board in two rows (A-F / a-f) */
   fprintf(f, "    A   B   C   D   E   F\n");
   fprintf(f, "   (%d) (%d) (%d) (%d) (%d) (%d)\n",
      board->board[0], board->board[1], board->board[2], board->board[3], board->board[4], board->board[5]);
   fprintf(f, "   (%d) (%d) (%d) (%d) (%d) (%d)\n",
      board->board[6], board->board[7], board->board[8], board->board[9], board->board[10], board->board[11]);
   fprintf(f, "    a   b   c   d   e   f\n");
   fprintf(f, "Captures: P1=%d P2=%d\n", board->player1_captures, board->player2_captures);
   fprintf(f, "Clockwise: %d\n", board->clockwise);
   fprintf(f, "=== GAME END ===\n\n");

   fclose(f);
   return 0;
}
