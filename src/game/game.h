#ifndef GAME_H
#define GAME_H

#define MAX_HISTORY_DEPTH 300 

typedef struct {
    int board[12];               // Current state of the board
    int player1_captures;        // Player 1's captured pebbles
    int player2_captures;        // Player 2's captured pebbles
    char move[50]; 
}BoardState;

typedef struct
{
    int board[12];
    int round;
    int player1_captures;
    int player2_captures;
    int clockwise;          //0 for counterclockwise, 1 for clockwise
    BoardState history[MAX_HISTORY_DEPTH]; 
    int history_size; 
}Board;


// Methods of the Board 'class'
Board* create_board();  // Constructor-like function to create and initialize the board
//void print_board(Board* board);  // Method to print the current state of the board
int player_turn(Board* board, int place);  // Method for handling a player's turn
int letter_to_int(char letter);  // Convert a letter to a board slot index
int game_over(Board* board);// Check if the game is over 
int choose_clockwise(Board* board, int clockwise);// Check if the current orientation is correct and sets it for the game. Return -1 for invalid input
void print_history(Board* board, char* player1_name, char* player2_name); //print the game history
#endif



/*Play Turn errors:
-1: Player can't play 
-2: Invalid position
-3: Empty pit
-4: Last pebble doesn't lands in the opponent's area in case of famine
-5: No pit can feed the opponent
*/