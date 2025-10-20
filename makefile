CC = gcc
CFLAGS = -Wall -Wextra -IClient -IServeur

# Executables
CLIENT_EXEC = client
SERVER_EXEC = server

# Source files
CLIENT_SRC = Client/client2.c
SERVER_SRC = Serveur/server2.c

# Object files
CLIENT_OBJ = $(CLIENT_SRC:.c=.o)
SERVER_OBJ = $(SERVER_SRC:.c=.o)

.PHONY: all clean

all: $(CLIENT_EXEC) $(SERVER_EXEC)

$(CLIENT_EXEC): $(CLIENT_OBJ)
	$(CC) $(CFLAGS) -o $@ $^

$(SERVER_EXEC): $(SERVER_OBJ)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f Client/*.o Serveur/*.o $(CLIENT_EXEC) $(SERVER_EXEC)
