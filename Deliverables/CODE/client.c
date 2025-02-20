#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/msg.h>

#define MSG_QUEUE_KEY 1234
#define MAX_MSG_LEN 256

typedef struct {
    long msg_type;
    char msg_text[MAX_MSG_LEN];
} msg_buffer;

int msgid;
char prompt[MAX_MSG_LEN] = "Enter command: ";  // Default prompt

void handle_shutdown(int sig) {
    printf("\nClient shutting down...\n");
    
    // Cleanup IPC
    if (msgctl(msgid, IPC_RMID, NULL) == -1) {
        perror("msgctl failed (client)");
    }

    exit(0);
}

void start_client(int client_id) {
    msgid = msgget(MSG_QUEUE_KEY, 0666 | IPC_CREAT);
    if (msgid == -1) {
        perror("msgget failed");
        exit(1);
    }

    msg_buffer msg;
    char command[MAX_MSG_LEN];

    // Send a REGISTER message to the server
    msg.msg_type = 1;
    snprintf(msg.msg_text, sizeof(msg.msg_text), "REGISTER %d", client_id);
    if (msgsnd(msgid, &msg, sizeof(msg.msg_text), 0) == -1) {
        perror("msgsnd failed");
    }
    if (msgrcv(msgid, &msg, sizeof(msg.msg_text), 2, 0) == -1) {
        perror("msgrcv failed");
    } else {
        if (strncmp(msg.msg_text, "REGISTER_SUCCESS", 16) == 0) {
            printf("Client %d successfully registered with the server.\n", client_id);
        } else {
            printf("Failed to register client. Exiting...\n");
            exit(1);
        }
    }

    while (1) {
        printf("%s", prompt);
        fgets(command, MAX_MSG_LEN, stdin);
        command[strcspn(command, "\n")] = '\0';  // Remove newline

        if (strcmp(command, "EXIT") == 0) {
            printf("Client exiting...\n");
            msg.msg_type = 1;
            strncpy(msg.msg_text, "EXIT", sizeof(msg.msg_text));
            msgsnd(msgid, &msg, sizeof(msg.msg_text), 0);
            break;
        }

        // Send command to the server
        msg.msg_type = 1;
        strncpy(msg.msg_text, command, sizeof(msg.msg_text));
        if (msgsnd(msgid, &msg, sizeof(msg.msg_text), 0) == -1) {
            perror("msgsnd failed");
        }

        // Wait for response
        if (msgrcv(msgid, &msg, sizeof(msg.msg_text), 2, 0) == -1) {
            perror("msgrcv failed");
        } else {
            if (strncmp(msg.msg_text, "SHUTDOWN", 8) == 0) {
                printf("Server is shutting down. Client exiting...\n");
                break;
            } 
            else if (strncmp(msg.msg_text, "Prompt changed to:", 18) == 0) {
                strncpy(prompt, msg.msg_text + 18, sizeof(prompt));
            }
            printf("Server response: %s\n", msg.msg_text);
        } 
    }

    handle_shutdown(SIGINT);
}

int main() {
    signal(SIGINT, handle_shutdown);

    int client_id = getpid();  // Unique client ID
    start_client(client_id);

    return 0;
}