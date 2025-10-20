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

int player_can_play(int player, Board* board) {
    int i, can_play = 0;
    if (player == 0) {
        while(i < sizeof(board->board)/(sizeof(int)*2) && !can_play ) {
            if (board->board[i] != 0) can_play = 1;
            i++;
        }
        
    } else {
        while(i < sizeof(board->board)/sizeof(int) && !can_play) {
            if (board->board[i] != 0) can_play = 1;
            i++;
        }
    }
    return can_play;
}

int player_turn(Board *board, int player, int place, int direction) {
    if (!player_can_play(player, board)) return -1;
    if (player == 1) {
        place += 6;
    }

    int pebbles = board->board[place];
    board->board[place] = 0;
    int iterator = place;

    // Pebble distribution
    while (pebbles > 0) {
        iterator = (iterator + direction + 12) % 12;

        //Do not distribute in the initial position
        if (iterator == place) {
            iterator = (iterator + direction + 12) % 12;
        }

        board->board[iterator] += 1;
        pebbles--;
    }

    int opponent_start = (player == 0) ? 6 : 0;
    int opponent_end   = (player == 0) ? 11 : 5;

    //We capture the oponent stashes containing 2-3 pebbles
    while (iterator >= opponent_start && iterator <= opponent_end) {

        int count = board->board[iterator];
        if (count == 2 || count == 3) {
            board->board[iterator] = 0;
            iterator = (iterator - direction + 12) % 12;
        } else {
            break;
        }
    }
    return 0;
}

int letter_to_int(char letter) {
    
    if (letter >= 'A' && letter <= 'Z') {
        letter = letter + ('a' - 'A');  
    }

    if (letter >= 'a' && letter <= 'z') {
        return letter - 'a';
    }

    // Invalid letter
    return -1;
}



int main() {
    printf("Game lauching...\n");
    Board board = create_board();
    print_board(&board);
    player_turn(&board, 1, 3, -1);
    print_board(&board);
    for(size_t i = 0; i < 6; i++) {
        board.board[i] = 0;
    }
    print_board(&board);
    printf("Player1 can play :%d\n", player_can_play(0, &board));
    printf("Player2 can play :%d\n", player_can_play(1, &board));
    printf("Player1 turn : %d", player_turn(&board, 0, 3, -1));
    return 0;
}