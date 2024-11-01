#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <errno.h>
#define BUFFER_SIZE 1024
#define FILENAME_MAX_LENGTH 256 // Increased filename buffer length
#define MSG_SIZE 15

struct user
{
    char username[50];
    char password[50];
};

// Function to receive data from the server
int receiveFileData(const char *fileName, const char *serverIP, int serverPort, struct user *user)
{
    int fileDescriptor = open("temp", O_CREAT | O_RDWR | O_TRUNC, 0644);
    if (fileDescriptor < 0)
    {
        perror("Error opening file for writing");
        return -1; // Failure
    }

    int socketFD, ret;
    struct sockaddr_in serverAddress;
    char buffer[BUFFER_SIZE] = {0};
    char responseMsg[MSG_SIZE];

    // Creating a socket
    if ((socketFD = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("Socket creation error");
        close(fileDescriptor);
        return -1; // Failure
    }

    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(serverPort);

    if (inet_pton(AF_INET, serverIP, &serverAddress.sin_addr) <= 0)
    {
        perror("Invalid address/ Address not supported");
        close(fileDescriptor);
        close(socketFD);
        return -1; // Failure
    }

    // Connecting to the server
    if (connect(socketFD, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0)
    {
        perror("Connection Failed");
        close(fileDescriptor);
        close(socketFD);
        return -1; // Failure
    }

    // Sending action request ("Fetch")
    ret = send(socketFD, "Fetch", strlen("Fetch"), 0);
    if (ret == -1)
    {
        perror("Failed to send Fetch request");
        close(fileDescriptor);
        close(socketFD);
        return -1; // Failure
    }

    ret = recv(socketFD, responseMsg, sizeof(responseMsg) - 1, 0);
    responseMsg[ret] = '\0';
    if (ret < 0 || strcmp(responseMsg, "Ok") != 0)
    {
        printf("Unexpected response: %s\n", responseMsg);
        close(fileDescriptor);
        close(socketFD);
        return -1;
    }

    char updatedName[150];
    strcpy(updatedName, user->username);
    strcat(updatedName, "/");
    strcat(updatedName, fileName);

    // Sending the file name to the server
    ret = send(socketFD, updatedName, strlen(updatedName), 0);
    if (ret == -1)
    {
        perror("Failed to send file name");
        close(fileDescriptor);
        close(socketFD);
        return -1; // Failure
    }

    // Receiving server response ("Ok" or "No file")
    ret = recv(socketFD, responseMsg, sizeof(responseMsg), 0);
    if (ret == -1)
    {
        perror("Failed to receive message");
        close(fileDescriptor);
        close(socketFD);
        return -1; // Failure
    }
    responseMsg[ret] = '\0';

    printf("1\n");

    if (strcmp("No file", responseMsg) == 0)
    {
        printf("File is not present on the server\n");
        close(fileDescriptor);
        close(socketFD);
        return 0;
    }
    printf("2\n");

    if (strcmp("No Read access", responseMsg) == 0)
    {
        printf("Permission denied\n");
        close(fileDescriptor);
        close(socketFD);
        return 0;
    }

    if (strcmp("Ok", responseMsg) != 0)
    {
        printf("Error: Server is not sending files\n");
        close(fileDescriptor);
        close(socketFD);
        return -1;
    }

    // Confirming to send the file
    ret = send(socketFD, "Send", strlen("Send"), 0);
    if (ret < 0)
    {
        printf("Error sending confirmation to receive file\n");
        close(fileDescriptor);
        close(socketFD);
        return -1;
    }

    // Receiving file data from the server
    printf("receiving the file data!");
    while ((ret = recv(socketFD, buffer, BUFFER_SIZE, 0)) > 0)
    {
        printf("buffer :%s\n", buffer);
        if (write(fileDescriptor, buffer, ret) <= 0)
        {
            perror("Error writing to the file");
            close(fileDescriptor);
            close(socketFD);
            return -1; // Failure
        }
        if (ret < BUFFER_SIZE)
        {
            break;
        }
    }

    if (ret < 0)
    {
        perror("Error receiving file data");
    }

    close(fileDescriptor);
    close(socketFD);
    return 1; // Success
}

// Function to send data to the server
int sendFileData(const char *fileName, const char *serverIP, int serverPort, struct user *user)
{
    int fileDescriptor = open("temp", O_RDONLY);
    if (fileDescriptor < 0)
    {
        perror("Error opening file for reading");
        return -1; // Failure
    }

    int socketFD, ret;
    struct sockaddr_in serverAddress;
    char buffer[BUFFER_SIZE] = {0};
    char responseMsg[5];

    // Creating a socket
    if ((socketFD = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("Socket creation error");
        close(fileDescriptor);
        return -1; // Failure
    }

    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(serverPort);

    if (inet_pton(AF_INET, serverIP, &serverAddress.sin_addr) <= 0)
    {
        perror("Invalid address/ Address not supported");
        close(fileDescriptor);
        close(socketFD);
        return -1; // Failure
    }

    // Connecting to the server
    if (connect(socketFD, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0)
    {
        perror("Connection Failed");
        close(fileDescriptor);
        close(socketFD);
        return -1; // Failure
    }

    // Sending action request ("Create/Store")
    ret = send(socketFD, "Create/Store", strlen("Create/Store"), 0);
    if (ret == -1)
    {
        perror("Failed to send Create/Store request");
        close(fileDescriptor);
        close(socketFD);
        return -1; // Failure
    }

    ret = recv(socketFD, responseMsg, sizeof(responseMsg) - 1, 0);
    responseMsg[ret] = '\0';
    if (strcmp("Ok", responseMsg) != 0)
    {
        printf("Error: Server did not acknowledge the request\n");
        close(fileDescriptor);
        close(socketFD);
        return -1;
    }

    // Sending the file name to the server
    char updatedName[150];
    strcpy(updatedName, user->username);
    strcat(updatedName, "/");
    strcat(updatedName, fileName);
    ret = send(socketFD, updatedName, strlen(updatedName), 0);
    if (ret == -1)
    {
        perror("Failed to send file name");
        close(fileDescriptor);
        close(socketFD);
        return -1; // Failure
    }

    // Receiving server response ("Ok")
    ret = recv(socketFD, responseMsg, sizeof(responseMsg), 0);
    if (ret == -1)
    {
        perror("Failed to receive message");
        close(fileDescriptor);
        close(socketFD);
        return -1; // Failure
    }

    if (strcmp(responseMsg, "Ok") != 0)
    {
        printf("Server is not accepting the file\n");
        close(fileDescriptor);
        close(socketFD);
        return -1; // Failure
    }

    // Sending file data to the server
    int bytesRead;
    while ((bytesRead = read(fileDescriptor, buffer, BUFFER_SIZE)) > 0)
    {
        ret = send(socketFD, buffer, bytesRead, 0);
        if (ret == -1)
        {
            perror("Failed to send file");
            close(fileDescriptor);
            close(socketFD);
            return -1; // Failure
        }
        if (bytesRead < BUFFER_SIZE)
        {
            break;
        }
    }

    char serverResponse[15];
    ret = recv(socketFD, serverResponse, sizeof(serverResponse), 0);
    if (ret == -1)
    {
        perror("Failed to receive message");
        close(fileDescriptor);
        close(socketFD);
        return -1; // Failure
    }
    serverResponse[ret] = '\0';
    printf("Server response: %s\n", serverResponse);
    close(fileDescriptor);
    close(socketFD);
    return 1; // Success
}

int executeCommand(const char *command)
{
    int returnValue = system(command);

    if (returnValue == -1)
    {
        perror("Error executing command");
        return 0; // Failure
    }

    return 1; // Success
}
int checkFilePresence(char *filename, const char *serverIP, int serverPort)
{

    char responseMsg[MSG_SIZE];
    int socketFD, ret;
    struct sockaddr_in serverAddress;
    char buffer[BUFFER_SIZE] = {0};

    // Creating a socket
    if ((socketFD = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("Socket creation error");
        return -1; // Failure
    }

    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(serverPort);

    if (inet_pton(AF_INET, serverIP, &serverAddress.sin_addr) <= 0)
    {
        perror("Invalid address/ Address not supported");
        close(socketFD);
        return -1; // Failure
    }

    // Connecting to the server
    if (connect(socketFD, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0)
    {
        perror("Connection Failed");
        close(socketFD);
        return -1; // Failure
    }
    ret = send(socketFD, "FilePresence", strlen("FilePresence"), 0);
    if (ret < 0)
    {
        perror("Sending Error \n");
        return 0;
    }
    ret = recv(socketFD, responseMsg, MSG_SIZE, 0);
    if (ret < 0)
    {
        perror("Error in reciving\n");
        return 0;
    }
    responseMsg[ret] = '\0';
    if (strcmp(responseMsg, "Filename") == 0)
    {
        ret = send(socketFD, filename, sizeof(filename), 0);
        if (ret < 0)
        {
            perror("Sending Error \n");
            return 0;
        }
        ret = recv(socketFD, responseMsg, MSG_SIZE, 0);
        if (ret < 0)
        {
            perror("Error in reciving\n");
            return 0;
        }
        responseMsg[ret] = '\0';
        if (strcmp(responseMsg, "No file") == 0)
        {
            return 0; // file not present
        }
        else
        {
            return 1; // file present
        }
    }
    else
    {

        perror("Something wrong in connection\n");
    }
}
