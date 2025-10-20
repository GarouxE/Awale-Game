#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "game.h"

Board create_board() {
    printf("Creation of the board...\n");
    Board board;
    for (size_t i = 0; i < sizeof(board.board)/sizeof(board.board[0]); i++ ) {
        board.board[i] = 4;
    }
    return board;
}

/* int player_can_play(int player, Board board) {
    int max_i;
    if (player == 0)
    for(int i = 0; i < max_i; i++) {

    }
} */

void print_board(Board* board) {
    printf("Printing of the board...\n\n");
    printf("    player1 \n");
    for (int i = 0; i < 6; i++) {
        printf("%c  ", 65+i);
    }
    printf("\n");
    for (int i = 0; i < 6; i++) {
        printf("%d  ", board->board[i]);
    }
    printf("\n\n");

    for (int i = 6; i < 12; i++) {
        printf("%d  ", board->board[i]);
    }
    printf("\n");
    for (int i = 0; i < 6; i++) {
        printf("%c  ",  97+i);
    }
    printf("\n");
    printf("    player2 \n");
    
}

int main() {
    printf("Game lauching...\n");
    Board board = create_board();
    print_board(&board);

    return 0;
}