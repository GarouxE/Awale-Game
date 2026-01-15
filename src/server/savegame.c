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
   fprintf(f, "Clockwise: %d\n", board->clockwise);
   /* Pretty-print board in two rows (A-F / a-f) */

   for (int i = 0; i < game->board->history_size; i++) {
        char frame[BUF_SIZE];
        frame[0] = '\0';

        fprintf(f,
            "\n========== GAME BOARD (Round %d) ==========\n"
            "          %s (P1)\n\n"
            "    A   B   C   D   E   F\n"
            "   (%d) (%d) (%d) (%d) (%d) (%d)\n"
            "   (%d) (%d) (%d) (%d) (%d) (%d)\n"
            "    a   b   c   d   e   f\n\n"
            "          %s (P2)\n\n"
            "Captures:\n"
            "  %s: %d\n"
            "  %s: %d\n"
            "Move played: %s\n"
            "================================\n",
            i + 1,
            game->player1.name,
            board->history[i].board[0], board->history[i].board[1], board->history[i].board[2],
            board->history[i].board[3], board->history[i].board[4], board->history[i].board[5],
            board->history[i].board[6], board->history[i].board[7], board->history[i].board[8],
            board->history[i].board[9], board->history[i].board[10], board->history[i].board[11],
            game->player2.name,
            game->player1.name, board->history[i].player1_captures,
            game->player2.name, board->history[i].player2_captures,
            board->history[i].move
        );

    }
   fprintf(f, "Captures: P1=%d P2=%d\n", board->player1_captures, board->player2_captures);
   fprintf(f, "=== GAME END ===\n\n");

   fclose(f);
   return 0;
}
