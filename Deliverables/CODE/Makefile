CC = gcc
CFLAGS = -Wall -pthread

SERVER_SRC = server.c
CLIENT_SRC = client.c
SERVER_OBJ = server.o
CLIENT_OBJ = client.o
SERVER_BIN = server
CLIENT_BIN = client

all: $(SERVER_BIN) $(CLIENT_BIN)

$(SERVER_BIN): $(SERVER_OBJ)
	$(CC) $(CFLAGS) -o $(SERVER_BIN) $(SERVER_OBJ)

$(CLIENT_BIN): $(CLIENT_OBJ)
	$(CC) $(CFLAGS) -o $(CLIENT_BIN) $(CLIENT_OBJ)

$(SERVER_OBJ): server.h
	$(CC) $(CFLAGS) -c $(SERVER_SRC)

$(CLIENT_OBJ): client.h
	$(CC) $(CFLAGS) -c $(CLIENT_SRC)

clean:
	rm -f $(SERVER_OBJ) $(CLIENT_OBJ) $(SERVER_BIN) $(CLIENT_BIN)
