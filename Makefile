CC = gcc
CFLAGS = -Wall -Wextra -I./src/client -I./src/server -I./src/game

# Dossiers
BIN_DIR   = bin
BUILD_DIR = build

# Executables
CLIENT_EXEC = $(BIN_DIR)/client
SERVER_EXEC = $(BIN_DIR)/server
GAME_EXEC   = $(BIN_DIR)/game

# Sources
CLIENT_SRC = src/client/client2.c
SERVER_SRC = src/server/server2.c
GAME_SRC   = src/game/game.c

# Headers
CLIENT_HEADERS = src/client/client2.h
SERVER_HEADERS = src/server/server2.h 
GAME_HEADERS   = src/game/game.h

# Objets
CLIENT_OBJ = $(BUILD_DIR)/client/client2.o
SERVER_OBJ = $(BUILD_DIR)/server/server2.o
GAME_OBJ   = $(BUILD_DIR)/game/game.o

.PHONY: all clean

# === Cible par défaut ===
all: $(BIN_DIR) $(CLIENT_EXEC) $(SERVER_EXEC) $(GAME_EXEC)

# Créer les dossiers bin et build
$(BIN_DIR):
	mkdir -p $(BIN_DIR)

$(BUILD_DIR)/client:
	mkdir -p $(BUILD_DIR)/client

$(BUILD_DIR)/server:
	mkdir -p $(BUILD_DIR)/server

$(BUILD_DIR)/game:
	mkdir -p $(BUILD_DIR)/game

# === Client ===
$(CLIENT_EXEC): $(CLIENT_OBJ)
	$(CC) $(CFLAGS) -o $@ $^

$(BUILD_DIR)/client/client2.o: $(CLIENT_SRC) $(CLIENT_HEADERS) | $(BUILD_DIR)/client
	$(CC) $(CFLAGS) -c $(CLIENT_SRC) -o $@

# === Server ===
$(SERVER_EXEC): $(SERVER_OBJ)
	$(CC) $(CFLAGS) -o $@ $^

$(BUILD_DIR)/server/server2.o: $(SERVER_SRC) $(SERVER_HEADERS) | $(BUILD_DIR)/server
	$(CC) $(CFLAGS) -c $(SERVER_SRC) -o $@

# === Game ===
$(GAME_EXEC): $(GAME_OBJ)
	$(CC) $(CFLAGS) -o $@ $^

$(BUILD_DIR)/game/game.o: $(GAME_SRC) $(GAME_HEADERS) | $(BUILD_DIR)/game
	$(CC) $(CFLAGS) -c $(GAME_SRC) -o $@

# === Clean ===
clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)
