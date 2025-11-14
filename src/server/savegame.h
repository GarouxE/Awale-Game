#ifndef SAVEGAME_H
#define SAVEGAME_H

#include "server.h"

int save_game(Client *player1, Client *player2, Board *board, Game *game);
void list_saved_games(Client* sender, char* response);
void view_saved_game(Client* sender, const char* buffer, char* response);

#endif /* SAVEGAME_H */
