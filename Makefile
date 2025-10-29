CC = gcc
CFLAGS = -Wall -Wextra -IClient -IServeur -IGame

# Dossiers
BIN_DIR = bin

# Executables
CLIENT_EXEC = $(BIN_DIR)/client
SERVER_EXEC = $(BIN_DIR)/server

# Sources
CLIENT_SRC = Client/client.c
SERVER_SRC = Serveur/server.c
GAME_SRC = Game/game.c

# Headers
CLIENT_HEADERS = Client/client.h
SERVER_HEADERS = Serveur/server.h Serveur/client.h
GAME_HEADERS = Game/game.h

# Objects
CLIENT_OBJ = Client/client.o
SERVER_OBJ = Serveur/server.o
GAME_OBJ = Game/game.o

.PHONY: all clean

# Main target
all: $(BIN_DIR) $(CLIENT_EXEC) $(SERVER_EXEC)

# Create bin directory if it doesn't exist
$(BIN_DIR):
	mkdir -p $(BIN_DIR)

# Client build
$(CLIENT_EXEC): $(CLIENT_OBJ) $(GAME_OBJ)
	$(CC) $(CFLAGS) -o $@ $^

$(CLIENT_OBJ): $(CLIENT_SRC) $(CLIENT_HEADERS)
	$(CC) $(CFLAGS) -c $(CLIENT_SRC) -o $@

# Server build
$(SERVER_EXEC): $(SERVER_OBJ) $(GAME_OBJ)
	$(CC) $(CFLAGS) -o $@ $^

$(SERVER_OBJ): $(SERVER_SRC) $(SERVER_HEADERS)
	$(CC) $(CFLAGS) -c $(SERVER_SRC) -o $@

# Game object compilation (do not create a separate game executable)
$(GAME_OBJ): $(GAME_SRC) $(GAME_HEADERS)
	$(CC) $(CFLAGS) -c $(GAME_SRC) -o $@

# Clean up
clean:
	rm -f Client/*.o Serveur/*.o Game/*.o $(BIN_DIR)/*
