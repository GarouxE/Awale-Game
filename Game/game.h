#ifndef GAME_H
#define GAME_H

typedef struct
{
    int board[12];
    int round;
    int player1_captures;
    int player2_captures;
    int clockwise;
}Board;

// Methods of the Board 'class'
Board* create_board();  // Constructor-like function to create and initialize the board
void print_board(Board* board);  // Method to print the current state of the board
int player_turn(Board* board, int place, int clockwise);  // Method for handling a player's turn
int letter_to_int(char letter);  // Convert a letter to a board slot index
int game_over(Board* board);// Check if the game is over 
#endif