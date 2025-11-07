CC = gcc
CFLAGS = -Wall -Wextra -Isrc/client -Isrc/server -Isrc/game

# Dossiers
SRC_DIR = src
BUILD_DIR = build
BIN_DIR = bin

# Executables
CLIENT_EXEC = $(BIN_DIR)/client
SERVER_EXEC = $(BIN_DIR)/server

# Sources
CLIENT_SRC = $(SRC_DIR)/client/client.c
SERVER_SRC = $(SRC_DIR)/server/server.c
GAME_SRC   = $(SRC_DIR)/game/game.c

# Headers
CLIENT_HEADERS = $(SRC_DIR)/client/client.h
SERVER_HEADERS = $(SRC_DIR)/server/server.h $(SRC_DIR)/server/client.h $(SRC_DIR)/game/game.h
GAME_HEADERS   = $(SRC_DIR)/game/game.h

# Objects
CLIENT_OBJ = $(BUILD_DIR)/client.o
SERVER_OBJ = $(BUILD_DIR)/server.o
GAME_OBJ   = $(BUILD_DIR)/game.o

.PHONY: all clean

all: $(BUILD_DIR) $(BIN_DIR) $(CLIENT_EXEC) $(SERVER_EXEC)

# Create directories if they don't exist
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

# Client build
$(CLIENT_EXEC): $(CLIENT_OBJ)
	$(CC) $(CFLAGS) -o $@ $^

$(BUILD_DIR)/client.o: $(CLIENT_SRC) $(CLIENT_HEADERS)
	$(CC) $(CFLAGS) -c $(CLIENT_SRC) -o $@

# Server build
$(SERVER_EXEC): $(SERVER_OBJ) $(GAME_OBJ)
	$(CC) $(CFLAGS) -o $@ $^

$(BUILD_DIR)/server.o: $(SERVER_SRC) $(SERVER_HEADERS)
	$(CC) $(CFLAGS) -c $(SERVER_SRC) -o $@

# Game library build
$(BUILD_DIR)/game.o: $(GAME_SRC) $(GAME_HEADERS)
	$(CC) $(CFLAGS) -c $(GAME_SRC) -o $@

clean:
	rm -f $(BUILD_DIR)/*.o $(BIN_DIR)/*
