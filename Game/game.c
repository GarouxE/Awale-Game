#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "game.h"

Board create_board() {
    printf("Creation of the board...\n");
    Board board;
    for (int i = 0; i < (int)(sizeof(board.board)/sizeof(int)); i++ ) {
        board.board[i] = 4;
    }
    return board;
}

void print_board(Board board) {
    printf("Printing of the board...\n");
    printf("player1: ");
    for (int i = 0; i < 6; i++) {
        printf("%d ", board.board[i]);
    }
    printf("\nplayer2: ");
    for (int i = 6; i < 12; i++) {
        printf("%d ", board.board[i]);
    }
}

int main() {
    printf("Game lauching...\n");
    Board board = create_board();
    print_board(board);

    return 0;
}