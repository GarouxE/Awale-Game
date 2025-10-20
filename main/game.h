#ifndef GAME_H
#define GAME_H

typedef struct
{
    int board[12];   
    int player;
    int round;
}Board;

Board create_board();
void print_board();


#endif