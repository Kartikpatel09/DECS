#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <pthread.h>
#include "serverHeader.h"



pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

typedef struct Node {
    int connection_fd;
    struct Node *next;
} Node;

Node *front = NULL;
Node *rear = NULL;

// Push a connection to the queue
void push(int connection_fd) {
    Node *newNode = (Node *)malloc(sizeof(Node));
    if (newNode == NULL) {
        printf("Memory allocation failed\n");
        return;
    }
    newNode->connection_fd = connection_fd;
    newNode->next = NULL;
    if (front == NULL) {
        front = newNode;
        rear = newNode;
    } else {
        rear->next = newNode;
        rear = newNode;
    }
}

// Pop a connection from the queue
int pop() {
    if (front == NULL) {
        return -1;
    } else {
        Node *temp = front;
        front = front->next;
        if (front == NULL) {
            rear = NULL;
        }
        int connection_fd = temp->connection_fd;
        free(temp);
        return connection_fd;
    }
}


// Worker thread to handle client requests
void *handleClientRequests() {
    char command[MSG_SIZE];
    while (1) {
        pthread_mutex_lock(&lock);
        while (front == NULL) {
            pthread_cond_wait(&cond, &lock);
        }
        pthread_mutex_unlock(&lock);

        int connection_fd;
        pthread_mutex_lock(&lock);
        connection_fd = pop();
        pthread_mutex_unlock(&lock);

        if (connection_fd == -1) {
            continue;
        }

        int result = recv(connection_fd, command, sizeof(command) - 1, 0);
        if (result <= 0) {
            printf("Error in receiving data\n");
            continue;
        }
        command[result] = '\0';
        if(strcmp(command,"Fetch")==0||strcmp(command,"Create/Store")==0){
            if(executeCommand(connection_fd,command)==-1){
                printf("Error has occured closing fd\n");
            }
        }
        else if(strcmp(command,"FilePresence")==0){
            result=send(connection_fd,"Filename",strlen("Filename"),0);
            if(result<0){perror("Error in sending ");break;}
            result=recv(connection_fd,command,MSG_SIZE,0);
            if(result<0){perror("Error in sending ");break;}
            command[result]='\0';
            if (access(command, F_OK) != 0) {
                send(connection_fd, "No file", strlen("No file"), 0);
                
            }
            else{
                send(connection_fd, "file", strlen("file"), 0);
            }

        }
        

        close(connection_fd);
        printf("Client handled by thread: %ld\n", (long)pthread_self());
        printf("-----------------------------------------------------------\n");
    }
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s <port> <num_threads>\n", argv[0]);
        return -1;
    }

    int port = atoi(argv[1]);
    int threadCount = atoi(argv[2]);
    pthread_t threads[threadCount];
    int server_socket, err;
    struct sockaddr_in server_addr;
    char *ip="127.0.0.1";

    for (int i = 0; i < threadCount; i++) {
        if ((err = pthread_create(&threads[i], NULL, handleClientRequests, NULL)) != 0) {
            printf("Error creating thread, error no: %d\n", err);
            return -1;
        }
    }

    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("Socket creation error\n");
        return -1;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    //server_addr.sin_addr.s_addr = INADDR_ANY;
    if(inet_pton(AF_INET,ip,&server_addr.sin_addr)<=0){
        printf("Invalid address/ Address not supported\n");
        return -1;
    }

    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Binding error: ");
        printf("Binding error\n");
        return -1;
    }

    if (listen(server_socket, MAX_CLIENTS) < 0) {
        printf("Error setting up listen\n");
        return -1;
    }

    printf("Multi-threaded server is waiting for client connections...\n");

    while (1) {
        struct sockaddr_in client_addr;
        int addrlen = sizeof(client_addr);
        int connection_fd = accept(server_socket, (struct sockaddr *)&client_addr, &addrlen);
        if (connection_fd < 0) {
            printf("Connection not established\n");
            continue;
        }

        pthread_mutex_lock(&lock);
        push(connection_fd);
        pthread_cond_signal(&cond);
        pthread_mutex_unlock(&lock);
    }

    return 0;
}
