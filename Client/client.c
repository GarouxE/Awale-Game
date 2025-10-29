#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "../Game/game.h"

#define PORT 8080
#define SERVER_IP "127.0.0.1"

int main() {
    int sock;
    struct sockaddr_in server_addr;
    char buffer[1024];
    char place_char;
    int clockwise;
    char message[256];  // Message buffer
    char move[2];  // Buffer for the move

    // Create socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);

    // Connect to server
    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        exit(EXIT_FAILURE);
    }

    // Game loop
    while (1) {
        // Clear the message buffer before receiving new data
        memset(message, 0, sizeof(message));

        // Receive the board from the server
        recv(sock, buffer, sizeof(Board), 0);
        Board *board = (Board *)buffer;  // Cast the buffer to the board structure

        // Receive the message (e.g., "Your turn" or "Waiting for Player X")
        recv(sock, message, sizeof(message), 0);

        // Print the board and status message
        print_board(board);
        printf("%s\n", message);

        // If it's the player's turn, ask for input
        if (strncmp(message, "Your turn", 9) == 0) {
            printf("Enter your move (A-F) and direction (0 for counterclockwise, 1 for clockwise): ");
            scanf(" %c", &place_char);
            scanf("%d", &clockwise);

            // Store the move in the buffer
            move[0] = place_char;
            move[1] = clockwise + '0';  // Convert to char

            // Send the move to the server
            send(sock, move, sizeof(move), 0);
        }
    }

    close(sock);
    return 0;
}
