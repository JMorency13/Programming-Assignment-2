#ifndef SERVER_H
#define SERVER_H

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#define MAX_CLIENTS 10
#define MAX_CMD_LEN 256
#define MSG_QUEUE_KEY 1234  // Unique key for message queue

// Struct to hold client information
typedef struct {
    int client_id;
    int is_hidden;
} client_info;

// Message structure for message queue communication
typedef struct msg_buffer {
    long msg_type;  // Message type (used for targeting specific clients)
    char msg_text[MAX_CMD_LEN];
} msg_buffer;

// Global variables
extern client_info clients[MAX_CLIENTS];
extern int client_count;

// Function declarations
void start_server(void);
void *client_handler(void *arg);
void execute_shell_command(char *command);
void handle_signal(int sig);

#endif // SERVER_H