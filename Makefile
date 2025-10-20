CC = gcc
CFLAGS = -Wall -Wextra -IClient -IServeur -IGame

# Dossiers
BIN_DIR = bin

# Executables
CLIENT_EXEC = $(BIN_DIR)/client
SERVER_EXEC = $(BIN_DIR)/server
GAME_EXEC = $(BIN_DIR)/game

# Sources
CLIENT_SRC = Client/client2.c
SERVER_SRC = Serveur/server2.c
GAME_SRC = Game/game.c

# Headers
CLIENT_HEADERS = Client/client2.h
SERVER_HEADERS = Serveur/server2.h Serveur/client2.h
GAME_HEADERS = Game/game.h

# Objects
CLIENT_OBJ = Client/client2.o
SERVER_OBJ = Serveur/server2.o
GAME_OBJ = Game/game.o

.PHONY: all clean

all: $(BIN_DIR) $(CLIENT_EXEC) $(SERVER_EXEC) $(GAME_EXEC)

# Create bin directory if it doesn't exist
$(BIN_DIR):
	mkdir -p $(BIN_DIR)

# Client build
$(CLIENT_EXEC): $(CLIENT_OBJ)
	$(CC) $(CFLAGS) -o $@ $^

Client/client2.o: $(CLIENT_SRC) $(CLIENT_HEADERS)
	$(CC) $(CFLAGS) -c $(CLIENT_SRC) -o $@

# Server build
$(SERVER_EXEC): $(SERVER_OBJ)
	$(CC) $(CFLAGS) -o $@ $^

Serveur/server2.o: $(SERVER_SRC) $(SERVER_HEADERS)
	$(CC) $(CFLAGS) -c $(SERVER_SRC) -o $@

# Game build
$(GAME_EXEC): $(GAME_OBJ)
	$(CC) $(CFLAGS) -o $@ $^

Game/game.o: $(GAME_SRC) $(GAME_HEADERS)
	$(CC) $(CFLAGS) -c $(GAME_SRC) -o $@

# Clean
clean:
	rm -f Client/*.o Serveur/*.o Game/*.o $(BIN_DIR)/*
