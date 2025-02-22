#include "server.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>

client_info clients[MAX_CLIENTS];
int client_count = 0;
int msgid = -1;
int running = 1;

void handle_signal(int sig) {
    printf("\nServer shutting down...\n");
    running = 0;

    if (msgid != -1) {  // Only try to send SHUTDOWN if queue exists
        msg_buffer shutdown_msg;
        shutdown_msg.msg_type = 1;
        strncpy(shutdown_msg.msg_text, "SHUTDOWN", sizeof(shutdown_msg.msg_text));
        msgsnd(msgid, &shutdown_msg, sizeof(shutdown_msg.msg_text), 0);

        if (msgctl(msgid, IPC_RMID, NULL) == -1) {
            perror("msgctl failed to remove queue");
        } else {
            msgid = -1;  // Prevent further msg operations
        }
    }

    exit(0);
}

void execute_shell_command(char *command) {
    if (command == NULL || strlen(command) == 0) {
        printf("Received an empty command.\n");
        return;
    }

    printf("Executing command: %s\n", command);  // Debugging print

    pid_t pid = fork();
    if (pid == 0) {
        // In child process: execute the command
        execlp("bash", "bash", "-c", command, NULL);
        perror("execlp failed");
        exit(1);
    } else if (pid > 0) {
        // In parent process: wait for the child process with timeout
        sleep(CMD_TIMEOUT);
        if (waitpid(pid, NULL, WNOHANG) == 0) {
            kill(pid, SIGKILL);
            printf("Command execution exceeded timeout. Process killed.\n");
        } else {
            printf("Command executed successfully.\n");
        }
    } else {
        perror("fork failed");
    }
}

void *client_handler(void *arg) {
    msg_buffer msg;

    while (running) {
        if (msgid == -1) {
            printf("Message queue deleted. Exiting handler.\n");
            pthread_exit(NULL);
        }

        if (msgrcv(msgid, &msg, sizeof(msg) - sizeof(long), 1, 0) == -1) {
            if (running) {
                perror("msgrcv failed");
            }
            continue;
        }

        printf("Received command: %s\n", msg.msg_text);

        if (strncmp(msg.msg_text, "REGISTER ", 9) == 0) {
            int new_client_id = atoi(msg.msg_text + 9); // Extract client ID

             // Check if the client is already registered
            int already_registered = 0;
            for (int i = 0; i < client_count; i++) {
                if (clients[i].client_id == new_client_id) {
                    already_registered = 1;
                    break;
                }
            }

            if (!already_registered) {
                if (client_count < MAX_CLIENTS) {
                    clients[client_count].client_id = new_client_id;
                    clients[client_count].is_hidden = 0; // Default to visible
                    client_count++;
                    printf("Client %d registered successfully.\n", new_client_id);
                    
                    // Send a response to the client confirming registration
                    msg.msg_type = 2;  // Set response type
                    snprintf(msg.msg_text, sizeof(msg.msg_text), "REGISTER_SUCCESS %d", new_client_id);
                    msgsnd(msgid, &msg, sizeof(msg.msg_text), 0);
                } else {
                    printf("Client registration failed: Maximum clients reached.\n");
                    
                    // Inform client that registration failed
                    msg.msg_type = 2;
                    strncpy(msg.msg_text, "REGISTER_FAILED", sizeof(msg.msg_text));
                    msgsnd(msgid, &msg, sizeof(msg.msg_text), 0);
                }
            }
            continue;
        }
        if (strncmp(msg.msg_text, "EXIT", 4) == 0) {
            int exiting_client_id = msg.msg_type;  // Use msg_type to identify client
            printf("Client %d exiting...\n", exiting_client_id);
        
            for (int i = 0; i < client_count; i++) {
                if (clients[i].client_id == exiting_client_id) {
                    for (int j = i; j < client_count - 1; j++) {
                        clients[j] = clients[j + 1];
                    }
                    client_count--;
                    break;
                }
            }
        
            if (client_count == 0) {
                printf("No more clients. Removing message queue.\n");
                if (msgctl(msgid, IPC_RMID, NULL) == -1) {
                    perror("msgctl failed to remove queue");
                } else {
                    msgid = -1;  // Prevent further operations on deleted queue
                }
            }
        
            pthread_exit(NULL);
        }

        if (strncmp(msg.msg_text, "LIST", 4) == 0) {
            char response[256] = "Connected clients: ";
            int found = 0;

            for (int i = 0; i < client_count; i++) {
                if (!clients[i].is_hidden) {
                    char client_str[10];
                    sprintf(client_str, "%d ", clients[i].client_id);
                    strcat(response, client_str);
                    found = 1;
                }
            }

            if (!found) {
                strncpy(response, "No clients currently visible.", sizeof(response));
            }

            msg.msg_type = 2;
            strncpy(msg.msg_text, response, sizeof(msg.msg_text));
            msgsnd(msgid, &msg, sizeof(msg.msg_text), 0);
            continue;
        }        
        else if (strncmp(msg.msg_text, "CHPT ", 5) == 0) {
            msg.msg_type = 2;
            snprintf(msg.msg_text, sizeof(msg.msg_text), "Prompt changed to: %s", msg.msg_text + 5);
        }
        else if (strncmp(msg.msg_text, "HIDE", 4) == 0) {
            int client_id = atoi(msg.msg_text + 5);  // Extract client ID from message
            for (int i = 0; i < client_count; i++) {
                if (clients[i].client_id == client_id) {
                    clients[i].is_hidden = 1;
                    snprintf(msg.msg_text, sizeof(msg.msg_text), "Client %d is now hidden.", client_id);
                    msgsnd(msgid, &msg, sizeof(msg.msg_text), 0);
                    break;
                }
            }
            continue;
        } 
        else if (strncmp(msg.msg_text, "UNHIDE", 6) == 0) {
            int client_id = atoi(msg.msg_text + 7);  // Extract client ID from message
            for (int i = 0; i < client_count; i++) {
                if (clients[i].client_id == client_id) {
                    clients[i].is_hidden = 0;
                    snprintf(msg.msg_text, sizeof(msg.msg_text), "Client %d is now visible.", client_id);
                    msgsnd(msgid, &msg, sizeof(msg.msg_text), 0);
                    break;
                }
            }
            continue;
        }
        else {
            execute_shell_command(msg.msg_text);
            msg.msg_type = 2;
            strncpy(msg.msg_text, "Command executed", sizeof(msg.msg_text));
        }

        if (msgsnd(msgid, &msg, sizeof(msg) - sizeof(long), 0) == -1) {
            if (running) {
                perror("msgsnd failed");
            }
        }
    }
    return NULL;
}

void start_server(void) {
    signal(SIGINT, handle_signal);

    msgid = msgget(MSG_QUEUE_KEY, 0666 | IPC_CREAT);
    if (msgid == -1) {
        perror("msgget failed");
        exit(1);
    }

    pthread_t thread;
    pthread_create(&thread, NULL, client_handler, NULL);
    pthread_detach(thread);

    while (running) {
        sleep(1);
    }
}

int main() {
    start_server();
    return 0;
}