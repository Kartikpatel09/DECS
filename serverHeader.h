#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <pthread.h>
#define MAX_CLIENTS 1024
#define BUFFER_SIZE 1024
#define FILENAME_MAX_LENGTH 256
#define MSG_SIZE 15

struct user
{
    char username[50];
    char password[50];
};
// Function to send file data to the client
int sendFileData(const char *filename, int connection_fd)
{
    int file_fd = open(filename, O_RDONLY);
    if (file_fd < 0)
    {
        perror("Error opening file");
        return -1;
    }

    char buffer[BUFFER_SIZE];
    int bytesRead, bytesSent;

    while ((bytesRead = read(file_fd, buffer, BUFFER_SIZE)) > 0)
    {
        bytesSent = send(connection_fd, buffer, bytesRead, 0);
        if (bytesSent < 0)
        {
            printf("Error in sending data\n");
            close(file_fd);
            return -1;
        }
        if (bytesRead < BUFFER_SIZE)
        {
            break;
        }
    }
    printf("File request satisfied\n");
    close(file_fd);
    return 1;
}

// Function to receive file data from the client
int receiveFileData(const char *filename, int connection_fd)
{
    int file_fd = open(filename, O_CREAT | O_TRUNC | O_RDWR, 0644);
    if (file_fd < 0)
    {
        perror("Error creating file");
        return -1;
    }

    char buffer[BUFFER_SIZE];
    int bytesReceived;

    while ((bytesReceived = recv(connection_fd, buffer, BUFFER_SIZE, 0)) > 0)
    {
        if (write(file_fd, buffer, bytesReceived) < 0)
        {
            printf("Error writing to file\n");
            close(file_fd);
            return -1;
        }
        if (bytesReceived < BUFFER_SIZE)
        {
            break;
        }
    }

    send(connection_fd, "File Saved", strlen("File Saved"), 0);
    printf("File saved\n");
    close(file_fd);

    return 1;
}
int executeCommand(int connection_fd, char *command)
{
    int result = send(connection_fd, "Ok", strlen("Ok"), 0);
    if (result <= 0)
    {
        printf("Error sending acknowledgment\n");
        return -1;
    }
    char filename[FILENAME_MAX_LENGTH];
    if (strcmp("Fetch", command) == 0)
    {
        result = recv(connection_fd, filename, FILENAME_MAX_LENGTH, 0);
        if (result <= 0)
        {
            printf("Error in receiving filename\n");
            return -1;
        }
        filename[result] = '\0';
        if (access(filename, F_OK) != 0)
        {
            send(connection_fd, "No file", strlen("No file"), 0);
            return -1;
        }
        else
        {
            send(connection_fd, "Ok", strlen("Ok"), 0);
            result = recv(connection_fd, command, sizeof(command), 0);
            command[result] = '\0';
            if (result < 0 || strcmp(command, "Send") != 0)
            {
                printf("Unexpected command received: %s\n", command);
                return -1;
            }
            sendFileData(filename, connection_fd);
        }
    }
    else if (strcmp("Create/Store", command) == 0)
    {
        result = recv(connection_fd, filename, FILENAME_MAX_LENGTH, 0);
        if (result <= 0)
        {
            printf("Error in receiving filename\n");
            return -1;
        }
        filename[result] = '\0';

        send(connection_fd, "Ok", strlen("Ok"), 0);
        receiveFileData(filename, connection_fd);
    }
    else
    {
        printf("Unknown command received: %s\n", command);
        return -1;
    }
    return 1;
}

void populateLogin(struct user *user, int connection_fd)
{
    int file_fd = open("login.dat", O_RDWR | O_APPEND);
    if (file_fd < 0)
    {
        perror("Error opening file");
        exit(0);
    }

    char buffer[101];
    int bytesRead, found = 0;

    while ((bytesRead = read(file_fd, buffer, 101)) > 0)
    {
        printf("user :%s\n", buffer);
        printf("pwd :%s\n", &buffer[50]);
        if (strcmp(buffer, user->username) == 0)
        {
            int result = send(connection_fd, "2", strlen("2"), 0);
            if (result < 0)
            {
                perror("Error in sending ");
            }
            found = 1;
            break;
        }
    }
    if (!found)
    {
        write(file_fd, user->username, sizeof(user->username));
        write(file_fd, user->password, sizeof(user->password));
        write(file_fd, "\n", 1);
        int result = send(connection_fd, "1", strlen("1"), 0);
        if (result < 0)
        {
            perror("Error in sending ");
        }
    }
    printf("File request satisfied\n");
    close(file_fd);
}

void checkLogin(struct user *user, int connection_fd)
{
    int file_fd = open("login.dat", O_RDONLY);
    if (file_fd < 0)
    {
        perror("Error opening file");
        exit(0);
    }

    char buffer[101];
    int bytesRead, found = 0;

    while ((bytesRead = read(file_fd, buffer, 101)) > 0)
    {
        if (strcmp(buffer, user->username) == 0 && strcmp(&buffer[50], user->password) == 0)
        {
            int result = send(connection_fd, "1", strlen("1"), 0);
            if (result < 0)
            {
                perror("Error in sending ");
            }
            found = 1;
            break;
        }
    }
    if (!found)
    {
        int result = send(connection_fd, "2", strlen("2"), 0);
        if (result < 0)
        {
            perror("Error in sending ");
        }
    }
    printf("File request satisfied\n");
    close(file_fd);
}