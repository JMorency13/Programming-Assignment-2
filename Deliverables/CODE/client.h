#ifndef CLIENT_H
#define CLIENT_H

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>

#define MAX_CMD_LEN 256
#define MSG_QUEUE_KEY 1234  // Same key as used in server

// Message structure
typedef struct msg_buffer {
    long msg_type;  // Message type
    char msg_text[MAX_CMD_LEN];
} msg_buffer;

// Function declarations
void start_client(int client_id);

#endif // CLIENT_H