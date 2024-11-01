#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <arpa/inet.h>
#define ENTER 10
#define TAB 9
#define BCKSPC 127
#define MSG_SIZE 15
#define BUFFER_SIZE 1024
#include "loginHeader.h"

struct user
{
    char username[50];
    char password[50];
};

void takePassword(char pwd[50])
{
    int i = 0;
    char ch;
    struct termios newt, oldt;
    do
    {
        tcgetattr(STDIN_FILENO, &oldt);
        newt = oldt;
        newt.c_lflag &= ~(ICANON | ECHO);
        tcsetattr(STDIN_FILENO, TCSANOW, &newt);
        printf("\nEnter Password:");
        while (1)
        {
            ch = getchar();
            if (ch == ENTER || ch == TAB)
            {
                pwd[i] = '\0';
                break;
            }
            else if (ch == BCKSPC)
            {
                if (i > 0)
                {
                    i--;
                    printf("\b \b");
                }
            }
            else
            {
                pwd[i++] = ch;
                printf("* \b");
            }
        }
        tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
        if (strlen(pwd) < 4)
            printf("\npassword is too weak, include more characters!\n");
        else
            break;
    } while (1);
    printf("\n");
}

void takeinput(struct user *user)
{
    char cnfpwd[50];
    printf("\nEnter userName:");
    fgets(user->username, 50, stdin);
    user->username[strlen(user->username) - 1] = 0;
    takePassword(user->password);
    do
    {
        printf("\nTo confirm again");
        takePassword(cnfpwd);
        if (strcmp(cnfpwd, user->password))
        {
            printf("Passwords doesn't match!\n");
            memset(cnfpwd, 0, 50);
        }
        else
            break;
    } while (1);
}

struct user *
loginPage(int serverPort, const char *serverIP)
{
    struct user *user = (struct user *)malloc(sizeof(struct user));
    int opt;

    printf("\n\t\t--------Login/Signup---------\n");
    printf("1. Signup\n");
    printf("2. Login\n");
    printf("3. Exit\n");
    printf("\nEnter your Choice : ");

    scanf("%d", &opt);
    fgetc(stdin);
    if (opt != 1 && opt != 2)
    {
        printf("Bye Bye!\n");
        exit(0);
    }
    int done = 0;

    do
    {
        char responseMsg[MSG_SIZE];
        int socketFD, ret;
        struct sockaddr_in serverAddress;
        char buffer[BUFFER_SIZE] = {0};

        // Creating a socket
        if ((socketFD = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        {
            perror("Socket creation error");
            exit(0); // Failure
        }

        serverAddress.sin_family = AF_INET;
        serverAddress.sin_port = htons(serverPort);

        if (inet_pton(AF_INET, serverIP, &serverAddress.sin_addr) <= 0)
        {
            perror("Invalid address/ Address not supported");
            close(socketFD);
            exit(0); // Failure
        }

        // Connecting to the server
        if (connect(socketFD, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0)
        {
            perror("Connection Failed");
            close(socketFD);
            exit(0); // Failure
        }

        // take the inputs
        takeinput(user);
        switch (opt)
        {
        case 1:

            ret = send(socketFD, "SignUp", strlen("SignUp"), 0);
            if (ret < 0)
            {
                perror("Sending Error \n");
                exit(0);
            }
            ret = recv(socketFD, responseMsg, MSG_SIZE, 0);
            if (ret < 0)
            {
                perror("Error in reciving\n");
                exit(0);
            }
            responseMsg[ret] = '\0';
            if (strcmp(responseMsg, "SignUp") == 0)
            {
                ret = send(socketFD, user, sizeof(struct user), 0);
                if (ret < 0)
                {
                    perror("Sending Error \n");
                    exit(0);
                }
                ret = recv(socketFD, responseMsg, MSG_SIZE, 0);
                if (ret < 0)
                {
                    perror("Error in reciving\n");
                    exit(0);
                }
                close(socketFD);
                responseMsg[ret] = '\0';
                if (strcmp(responseMsg, "1") == 0)
                {
                    printf("User created and logged in!\n");
                    done = 1;
                }
                else
                {
                    printf("user already exists, try new username!\n");
                }
            }
            else
            {

                perror("Something wrong in connection\n");
            }
            close(socketFD);
            break;
        case 2:
            ret = send(socketFD, "Login", strlen("Login"), 0);
            if (ret < 0)
            {
                perror("Sending Error \n");
                exit(0);
            }
            ret = recv(socketFD, responseMsg, MSG_SIZE, 0);
            if (ret < 0)
            {
                perror("Error in reciving\n");
                exit(0);
            }
            responseMsg[ret] = '\0';
            if (strcmp(responseMsg, "Login") == 0)
            {
                ret = send(socketFD, user, sizeof(struct user), 0);
                if (ret < 0)
                {
                    perror("Sending Error \n");
                    exit(0);
                }
                ret = recv(socketFD, responseMsg, MSG_SIZE, 0);
                if (ret < 0)
                {
                    perror("Error in reciving\n");
                    exit(0);
                }
                close(socketFD);
                responseMsg[ret] = '\0';
                if (strcmp(responseMsg, "1") == 0)
                {
                    printf("User logged in!\n");
                    done = 1;
                }
                else
                {
                    printf("Invalid user or password!\nLogin again");
                }
            }
            else
            {
                perror("Something wrong in connection\n");
            }
            close(socketFD);
            break;
        }
    } while (done == 0);
    return user;
}