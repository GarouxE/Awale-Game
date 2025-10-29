#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "../Game/game.h"

#define PORT 8080
#define MAX_CLIENTS 2

// Function to send the board with a message to a player
void send_board_with_message(int socket, Board *board, const char *message) {
    // Send the board first
    send(socket, board, sizeof(Board), 0);
    
    // Send the message (e.g., "Your turn" or "Waiting for Player X")
    send(socket, message, strlen(message) + 1, 0);  // +1 to send the null terminator
}

int main() {
    int server_fd, new_sock, new_sock2;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    char buffer[1024];
    char move[2];  // Buffer to receive the move (place + direction)
    
    // Create socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    // Bind socket
    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    // Listen for connections
    if (listen(server_fd, MAX_CLIENTS) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Server waiting for players...\n");

    // Accept player 1
    if ((new_sock = accept(server_fd, (struct sockaddr *)&client_addr, &client_len)) < 0) {
        perror("Accept failed");
        exit(EXIT_FAILURE);
    }
    printf("Player 1 connected!\n");

    // Accept player 2
    if ((new_sock2 = accept(server_fd, (struct sockaddr *)&client_addr, &client_len)) < 0) {
        perror("Accept failed");
        exit(EXIT_FAILURE);
    }
    printf("Player 2 connected!\n");

    // Initialize game board
    Board* board = create_board();


    // Main game loop
    while (!game_over(board)) {
         int res = 3;
         // Player 1's turn
         printf("Player 1's turn...\n");

         send_board_with_message(new_sock, board, "Your turn");
         send_board_with_message(new_sock2, board, "Waiting for Player 1");

         // Receive Player 1's move (place + direction)
         recv(new_sock, move, sizeof(move), 0);
         int place = move[0] - 'A';  // Assume valid input
         int clockwise = move[1] - '0';  // Assume valid input
         res = player_turn(board, place, clockwise);

         // Send updated board to Player 1 (they just made a move)
         send_board_with_message(new_sock2, board, "Your turn");
         send_board_with_message(new_sock, board, "Waiting for Player 2");

         // Player 2's turn
         printf("Player 2's turn...\n");

         // Receive Player 2's move (place + direction)
         recv(new_sock2, move, sizeof(move), 0);
         place = move[0] - 'A';  // Assume valid input
         clockwise = move[1] - '0';  // Assume valid input
         res = player_turn(board, place, clockwise);
    }

    // Game over, determine winner
    printf("Game Over!\n");

    close(new_sock);
    close(new_sock2);
    close(server_fd);

    return 0;
}
