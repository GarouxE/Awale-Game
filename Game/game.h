#ifndef GAME_H
#define GAME_H

typedef struct
{
    int board[12];
    int round;
    int player1_captures;
    int player2_captures;
}Board;

// Methods of the Board 'class'
Board* create_board();  // Constructor-like function to create and initialize the board
void print_board(Board* board);  // Method to print the current state of the board
int player_turn(Board* board, int place, int clockwise);  // Method for handling a player's turn
int letter_to_int(char letter);  // Convert a letter to a board slot index
/*
static int player_can_play(int player, Board* board);  // Method to check if a player can play. Returns -1 if there is no calid position. 0 otherwise.
static int check_player_move(Board* board, int place); // ckecks if the move of the player can be done. Returns -2 for invalid position / -3 for an empty cell / 0 if everything is ok.
static int check_clockwise(int clockwise); //Function to check if the clockwise value is valid. Returns -4 if invalid
static int player_can_capture( Board* board);  // Check if the current player can make a capturing move
static int check_player_validity(Board* board, int place, int clockwise, int player);// Function to check if the move is valid and if the player can play
static int distribute_pebbles(Board* board, int* order, int place, int start_index); // Function to distribute pebbles based on the starting position and direction
static int find_starting_index(int* order, int place);// Function to find the starting index in the order array
static int collect_captures(Board* board, int player, int* order, int i);// Function to collect captured pebbles based on the final position
static int simulate_collect_captures(Board* board, int player, int* order, int i); //checks if after a move the opponent is in famine or no.*/
#endif