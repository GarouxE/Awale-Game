#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include "game.h"


//______________________________Checks______________________________
// Check if the current player can make a move (all cells different from 0)
static int player_can_play(int player, Board* board) {
    int i = (player == 0) ? 0 : 6;
    int end = (player == 0) ? 6 : 12;
    int can_play = 0;

    // Only check the appropriate player's side (0-5 or 6-11)
    for (; i < end && !can_play; i++) {
        if (board->board[i] > 0) {
            can_play = 1;
        }
    }
    if(can_play) return 1;
    return -1;
}

// Check if the move is valid (1-6 and on a not empty cell)
static int check_player_move(Board* board, int place){
    if (place == -1 || place > 5) {
        //printf("Invalid position! Please choose a valid position (A-F or a-f).\n");
        return -2;  // Invalid position
    }

    int player = board->round % 2;
    // Check if the current player selected an empty pit
    if ((player == 0 &&  board->board[place] == 0) || 
        (player == 1 && board->board[place + 6] == 0)) {
        //printf("Invalid move! The chosen pit is empty. Choose another pit with pebbles.\n");
        return -3;  //Empty pit
    }

    return 1;  // Valid move
}

//Check if the clockwise value is valid (== 0 or ==1)
static int check_clockwise(int clockwise) {
    if (clockwise != 0 && clockwise != 1) {
        //printf("Invalid direction! Please enter 0 for counterclockwise or 1 for clockwise.\n");
        return -4;  // Return -4 for invalid input
    }
    return 1;  // Return valid 
}

// Check if the current player can make a capturing move
static int player_can_capture(Board* board) {
    int player = board->round % 2;
    int i = (player == 0) ? 0 : 6;
    int end = (player == 0) ? 6 : 12;

    for (; i < end; i++) {
        if (board->board[i] > 0) {  // Player has pebbles to move
            int pebbles = board->board[i];
            int idx = i;
            int temp_board[12];
            memcpy(temp_board, board->board, sizeof(temp_board));
            temp_board[i] = 0;

            // Simulate pebble distribution
            while (pebbles > 0) {
                idx = (idx + 1) % 12;
                if ((player == 0 && idx == 6) || (player == 1 && idx == 12)) {
                    continue;  // Skip opponent's store
                }
                temp_board[idx]++;
                pebbles--;
            }

            // Check for captures in opponent's pits
            int capture_start = (player == 0) ? 6 : 0;
            int capture_end = (player == 0) ? 12 : 6;

            for (int j = capture_start; j < capture_end; j++) {
                if (temp_board[j] == 2 || temp_board[j] == 3) {
                    return 1;  // Capture possible
                }
            }
        }
    }

    return -5;  // No captures possible
}

// Check the distribution to ensure it lands in the opponent's area in case of famine
static int simulate_distribution_and_famine(Board* board, int player, int place, int clockwise) {
    if(player_can_play( (player+1) %2 , board ) ) return 1;
    // Define start and end indexes for each player
    int start = (player == 0) ? 0 : 6;
    int end = (player == 0) ? 6 : 12;

    // Get the number of pebbles in the selected pit
    int pebbles = board->board[place];
    int idx = place;
    
    // Simulate the distribution (without changing the board)
    for (int i = 0; i < pebbles; i++) {
        // Move to the next index based on the direction
        if (clockwise) {
            idx = (idx + 1) % 12;  // Clockwise
        } else {
            idx = (idx - 1 + 12) % 12;  // Counterclockwise
        }

        // Skip the opponent's store (index 6 or 12)
        if ((player == 0 && idx == 6) || (player == 1 && idx == 12)) {
            i--;  // Don't distribute a pebble into the store
        }
    }

    // After the loop, the idx will be where the last pebble lands
    // Check if the last pebble lands on the opponent's side
    if ((player == 0 && idx >= 6 && idx < 12) || (player == 1 && idx >= 0 && idx < 6)) {
        return 1;  // The pebble lands in the opponent's area
    }

    return -7;  // The last pebble does not land in the opponent's area
}

//Check id a player can feed the opponent
static int player_can_feed_opponent(Board* board,int player) {
    int start = (player == 0) ? 0 : 6;  // Player 1: pits 0-5, Player 2: pits 6-11
    int end = (player == 0) ? 6 : 12;   // Player 1: ends at 6, Player 2: ends at 12
    
    // Check each pit in the player's side
    for (int i = start; i < end; i++) {
        if (board->board[i] > 0) {  // If the player has pebbles in this pit
            int pebbles = board->board[i];  // Number of pebbles in the pit
            
            // Calculate the final position of the last pebble in clockwise distribution
            int final_pos_clockwise = (i + pebbles) % 12;
            
            // If the last pebble lands in the opponent's area, return 1 (can feed the opponent)
            if ((player == 0 && final_pos_clockwise >= 6 && final_pos_clockwise < 12) ||
                (player == 1 && final_pos_clockwise >= 0 && final_pos_clockwise < 6)) {
                return 1;
            }

            // Calculate the final position of the last pebble in counterclockwise distribution
            int final_pos_counterclockwise = (i - pebbles + 12) % 12;
            
            // If the last pebble lands in the opponent's area, return 1 (can feed the opponent)
            if ((player == 0 && final_pos_counterclockwise >= 6 && final_pos_counterclockwise < 12) ||
                (player == 1 && final_pos_counterclockwise >= 0 && final_pos_counterclockwise < 6)) {
                return 1;
            }
        }
    }
    
    // If no pit can feed the opponent, return -6
    return -6;
}

// Function to check if the move is valid and if the player can play
static int check_player_validity(Board* board, int place, int clockwise, int player) {
    int err = player_can_play(player, board);
    if (err != 1) return err;  // No valid move

    // Check if the move is valid
    err = check_player_move(board, place);
    if (err != 1 ) return err;  // Invalid position or empty pit

    // Check if clockwise direction is valid
    err = check_clockwise(clockwise);
    if (err != 1) return err;  // Invalid clockwise entry

    err = simulate_distribution_and_famine(board, player, place,clockwise);
    if (err != 1) return err; // The opponent is famined and you don't feed him


    return 1;  // Everything is valid
}

// Check if the game is over 
int game_over(Board* board) {
    // Check if either player has captured at least 25 pebbles
    if (board->player1_captures >= 25) {
        //printf("Player 1 wins by capturing 25 or more pebbles!\n");
        return 1;  // Player 1 wins
    } else if (board->player2_captures >= 25) {
        //printf("Player 2 wins by capturing 25 or more pebbles!\n");
        return 1;  // Player 2 wins
    }

    // Check if both players cannot make a move
    if (!player_can_play(0, board) && !player_can_play(1, board)) {
        //printf("Game over! Both players cannot make a move.\n");
        return 1;  // No more moves possible
    }

    // Check for famine condition where a player cannot nourish the opponent
    if (!player_can_play((board->round + 1) % 2, board) && !player_can_feed_opponent(board,(board->round % 2))) {
        // If so, the player who can still play takes all of their remaining pebbles
        int player = board->round % 2;  // Determine the current player
        int start = (player == 0) ? 0 : 6;  // Player 1: pits 0-5, Player 2: pits 6-11
        int end = (player == 0) ? 6 : 12;   // Player 1: ends at 6, Player 2: ends at 12
        
        // Collect all the remaining pebbles for the player who can still play
        for (int i = start; i < end; i++) {
            if (board->board[i] > 0) {
                // Add the remaining pebbles in this pit to the player's capture
                if (player == 0) {
                    board->player1_captures += board->board[i];
                } else {
                    board->player2_captures += board->board[i];
                }
                // Set the pit to 0 since all pebbles are taken
                board->board[i] = 0;
            }
        }
        return 1;  // The game is still ongoing
    }


    return 0;  // The game is still ongoing
}


//______________________________Utils______________________________
// Convert a letter (A-Z or a-z) to an index (0-11)
int letter_to_int(char letter) {
    if (letter >= 'A' && letter <= 'Z') {
        letter = letter + ('a' - 'A');  // Convert to lowercase
    }
    if (letter >= 'a' && letter <= 'z') {
        return letter - 'a';  // Return the correct index
    }
    return -1;  // Invalid letter
}

// Function to distribute pebbles based on the starting position and direction
static int distribute_pebbles(Board* board, int* order, int place, int start_index) {
    int pebbles = board->board[place];
    board->board[place] = 0;

    int i = start_index;
    while (pebbles > 0) {
        i = (i + 1) % 12;
        if (order[i] == place) {
            i = (i + 1) % 12;  // Skip the starting position
        }
        board->board[order[i]] += 1;
        pebbles--;
    }

    return i;  // Return the final index after distribution
}

// Function to find the starting index in the order array
static int find_starting_index(int* order, int place) {
    for (int i = 0; i < 12; i++) {
        if (order[i] == place) {
            return i;
        }
    }
    return -1;  // Invalid position, should not happen
}

// Function to collect captured pebbles based on the final position
static int collect_captures(Board* board, int player, int* order, int i) {
    int iterator = order[i];
    int opponent_start = (player == 0) ? 6 : 0;
    int opponent_end = (player == 0) ? 11 : 5;

    // Check if capture can be made
    while (iterator >= opponent_start && iterator <= opponent_end) {
        if (board->board[iterator] == 2 || board->board[iterator] == 3) {
            int captured_pebbles = board->board[iterator];
            board->board[iterator] = 0;

            // Add captured pebbles to current player
            if (player == 0) {
                board->player1_captures += captured_pebbles;
            } else {
                board->player2_captures += captured_pebbles;
            }

            i = (i - 1 + 12) % 12;
            iterator = order[i];
        } else {
            break;
        }
    }

    return i;  // Return the final index after capturing
}

static int simulate_collect_captures(Board* board, int player, int* order, int i) {
    int iterator = order[i];
    int opponent_start = (player == 0) ? 6 : 0;
    int opponent_end = (player == 0) ? 11 : 5;
    
    int sim_board[12];
    for (int j = 0; j < 12; j++) {
        sim_board[j] = board->board[j];  // Copy the board to sim_board
    }

    // Check if capture can be made
    while (iterator >= opponent_start && iterator <= opponent_end) {
        if (sim_board[iterator] == 2 || sim_board[iterator] == 3) {
            sim_board[iterator] = 0;

            i = (i - 1 + 12) % 12;
            iterator = order[i];
        } else {
            break;
        }
    }
    // Check if the opponent can still play after the simulated capture
    int opponent_can_play = -1;
    for (int j = opponent_start; j <= opponent_end; j++) {
        if (sim_board[j] > 0) {  // Opponent can play if there are pebbles in any pit
            opponent_can_play = 1;
            break;
        }
    }

    // Return the final index after simulating the capture
    return opponent_can_play;
}

//______________________________Application______________________________
// Create and initialize the game board
Board* create_board() {
    printf("Creation of the board...\n");

    // Dynamically allocate memory for the board
    Board* board = (Board*)malloc(sizeof(Board));
    if (!board) {
        printf("Memory allocation failed!\n");
        return NULL;  // Return NULL if memory allocation fails
    }

    // Initialize the board
    for (int i = 0; i < 12; i++) {
        board->board[i] = 4;  // Initialize each position with 4 pebbles
    }
    board->round = 0;   // Start from round 0 and also determines which player's turn it is (even number -> player 1 / uneven number -> player 2)
    board->player1_captures = 0;  // Initialize captures for player 1
    board->player2_captures = 0;  // Initialize captures for player 2
    return board;
}

// Print the current state of the board, player, and round
void print_board(Board* board) {
    printf("\n\nRound: %d, Current Player: %d\n", board->round, (board->round % 2) + 1);  // Player 1 or 2
    
    printf("Printing the board...\n\n");
    printf("    player1 \n");
    for (int i = 0; i < 6; i++) {
        printf("%c  ", 65 + i);  // A to F
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
        printf("%c  ", 97 + i);  // a to f
    }
    printf("\n");
    printf("    player2 \n");

    // Display current captures
    printf("Player 1 Captures: %d\n", board->player1_captures);
    printf("Player 2 Captures: %d\n", board->player2_captures);
}

// Execute a player's turn (distribute pebbles, handle capture, and update player/round)
int player_turn(Board* board, int place, int clockwise) {
    int player = board->round % 2;

    // Check player validity (can play, valid move, and valid clockwise direction)
    int pass = check_player_validity(board, place, clockwise, player);
    if (pass != 1) return pass;

    // Personalized orders based on clockwise or counterclockwise direction
    int order_clockwise[12] = {0, 1, 2, 3, 4, 5, 11, 10, 9, 8, 7, 6};
    int order_counterclockwise[12] = {6, 7, 8, 9, 10, 11, 5, 4, 3, 2, 1, 0};
    int* order = clockwise ? order_clockwise : order_counterclockwise;

    // Adjust place for Player 2 (positions 6-11) based on player
    if (player == 1) {
        place += 6;
    }

    // Find the starting index in the order array
    int start_index = find_starting_index(order, place);
    if (start_index == -1) {
        printf("Invalid starting position!\n");
        return -1;  // Invalid start position
    }

    // Distribute the pebbles and get the final index
    int final_index = distribute_pebbles(board, order, place, start_index);

    // Collect captured pebbles after distribution
    int verification = simulate_collect_captures(board, player, order, final_index);
    if(verification) final_index = collect_captures(board, player, order, final_index);

    // Update the round and change the player
    board->round += 1;

    return 0;  // Successfully executed the player's turn
}


int main() {
    printf("Game launching...\n");

    // Create the board
    Board* board = create_board();
    if (!board) {
        return -1;  // Exit if board creation failed
    }
    print_board(board);

    // Game loop
    while (!game_over(board)) {
        // Print the board and ask for a move
        print_board(board);

        int player = board->round % 2;  // Determine which player's turn
        int place;
        char place_char;

        printf("Player %d, enter the position: ", player + 1);
        scanf(" %c", &place_char);  // Read a letter (A-F or a-f)
        
        place = letter_to_int(place_char);  // Convert the letter to an index

        int clockwise;
        printf("Enter direction (0 for counterclockwise, 1 for clockwise): ");
        scanf("%d", &clockwise);

        // Execute the player's turn
        int turn = player_turn(board, place, clockwise);
        if (turn == -1) {
            printf("Player %d cannot make a move.\n", player + 1);
            continue;
        }
        if(turn == -2) printf("Invalid position! Please choose a valid position.\n");
        if(turn == -3) printf("Empty cell! Please choose a valid position.\n");
        if(turn == -4) printf("Not a valid entry for the clockwise value! Please choose a valid position.\n");
    }

    // Game over, print the final scores
    printf("\nGame over! Final scores:\n");
    printf("Player 1 captures: %d\n", board->player1_captures);
    printf("Player 2 captures: %d\n", board->player2_captures);

    if (board->player1_captures > board->player2_captures) {
        printf("Player 1 wins!\n");
    } else if (board->player2_captures > board->player1_captures) {
        printf("Player 2 wins!\n");
    } else {
        printf("It's a tie!\n");
    }

    // Don't forget to free the dynamically allocated memory for the board
    free(board);

    return 0;
}
