#include "headers.h"
#include <time.h>
#define BUFFER_SIZE 1024
#define FILENAME_MAX_LENGTH 128 // Increased filename buffer length
#define MSG_SIZE 20
#define COMMAND_SIZE 620
#define TIMESTAMP_LENGTH 20
struct user
{
    char username[50];
    char password[50];
};
typedef struct{
    char filename[FILENAME_MAX_LENGTH];
    char timestamp[20];
}cached_data;

int executeCommand(char *command);
//-------------------------------------------------------Caching Functions--------------------------------------------

int slot=0;
int clear_cache_flag=0;
int compareTime(char *time1, char *time2) {
    struct tm time_1 = {0};
    struct tm time_2 = {0};

    // Parse time1
    if (sscanf(time1, "%d-%d-%d %d:%d:%d",
               &time_1.tm_year, &time_1.tm_mon, &time_1.tm_mday,
               &time_1.tm_hour, &time_1.tm_min, &time_1.tm_sec) != 6) {
        printf("Error parsing time1\n");
        return -2;
    }

   
    time_1.tm_year -= 1900; 
    time_1.tm_mon -= 1;     
    
    if (sscanf(time2, "%d-%d-%d %d:%d:%d",
               &time_2.tm_year, &time_2.tm_mon, &time_2.tm_mday,
               &time_2.tm_hour, &time_2.tm_min, &time_2.tm_sec) != 6) {
        printf("Error parsing time2\n");
        return -2;
    }

    
    time_2.tm_year -= 1900; // tm_year is years since 1900
    time_2.tm_mon -= 1;     // tm_mon is 0-11


    time_t firstTime = mktime(&time_1);
    time_t secondTime = mktime(&time_2);

    if (firstTime == -1 || secondTime == -1) {
        printf("Error in struct tm to time_t conversion\n");
        return -2;
    }

    // Compare the two times
    if (firstTime > secondTime) {
        return 1;
    } else if (firstTime < secondTime) {
        return -1;
    } else {
        return 0;
    }
}


int Insert(char *filename, char *timestemp,int flag) {
    int fileDescriptor = open("_cache_/access.dat", O_RDWR);
    if (fileDescriptor < 0) {
        perror("Error opening file");
        return 0;
    }
    int possition=slot*149;
    if(flag!=-1){
        possition=flag*149;
    }
    printf("\n%d\n",possition);
    if (lseek(fileDescriptor, possition, SEEK_SET) == -1) {
        perror("Error seeking in file");
        close(fileDescriptor);
        return 0;
    }
    slot = (slot + 1) % 4;
    if(slot==0){
        clear_cache_flag=1;
    }
    // Write filename
    if (write(fileDescriptor, filename, 128) < 0) {
        perror("Error writing filename");
        close(fileDescriptor);
        return 0;
    }

    // Write timestamp 
    if (write(fileDescriptor, timestemp, 20) < 0) {
        perror("Error writing timestamp");
        close(fileDescriptor);
        return 0;
    }

    // Write newline character to separate entries
    if (write(fileDescriptor, "\n", 1) < 0) {
        perror("Error writing newline");
        close(fileDescriptor);
        return 0;
    }

    close(fileDescriptor);
    return 1;
}
int search(char* filename){
    int index = 0;
    int fileDescriptor = open("_cache_/access.dat", O_RDONLY);
    if (fileDescriptor < 0) {
        perror("Error opening file");
        return -1;
    }

    char buff[149];

    // Read 149 bytes at a time to locate the filename
    while (read(fileDescriptor, buff, 149) > 0) {
        
        if (strncmp(buff, filename, strlen(filename)) == 0) {
            close(fileDescriptor);  
            return index;
        }
        index++;
    }
    close(fileDescriptor);
    return -1;

}

int isCached(char *filename, const char *serverIP, int serverPort, struct user *user) {
    char timestamp[TIMESTAMP_LENGTH];
    int index = search(filename);
    char storedTime[TIMESTAMP_LENGTH] = {0}; 
    char buff[149];

    // Create socket and connect to the server
    int socketFD = socket(AF_INET, SOCK_STREAM, 0);
    if (socketFD < 0) {
        perror("Socket creation error");
        return -1; 
    }

    struct sockaddr_in serverAddress = {0};
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(serverPort);

    if (inet_pton(AF_INET, serverIP, &serverAddress.sin_addr) <= 0) {
        perror("Invalid address/ Address not supported");
        close(socketFD);
        return -1; 
    }

    // Connecting to the server
    if (connect(socketFD, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0) {
        perror("Connection Failed");
        close(socketFD);
        return -1; 
    }

    // Send action request ("Fetch")
    if (send(socketFD, "Fetch", strlen("Fetch"), 0) == -1) {
        perror("Failed to send Fetch request");
        close(socketFD);
        return -1; 
    }

    // Receive response
    char responseMsg[MSG_SIZE];
    int ret = recv(socketFD, responseMsg, sizeof(responseMsg) - 1, 0);
    if (ret < 0) {
        perror("Failed to receive message");
        close(socketFD);
        return -1; 
    }
    responseMsg[ret] = '\0';

    if (strcmp(responseMsg, "Ok") != 0) {
        printf("Unexpected response: %s\n", responseMsg);
        close(socketFD);
        return -1;
    }

    char updatedName[150];
    snprintf(updatedName, sizeof(updatedName), "%s/%s", user->username, filename);

    // Send filename to server
    if (send(socketFD, updatedName, strlen(updatedName), 0) == -1) {
        perror("Failed to send file name");
        close(socketFD);
        return -1; 
    }

    // Receive server response
    ret = recv(socketFD, responseMsg, sizeof(responseMsg), 0);
    if (ret < 0) {
        perror("Failed to receive message");
        close(socketFD);
        return -1; 
    }
    responseMsg[ret] = '\0';

    if (strcmp("No file", responseMsg) == 0) {
        printf("File is not present on the server\n");
        close(socketFD);
        return 0;
    } else if (strcmp("No Read access", responseMsg) == 0) {
        printf("Permission denied\n");
        close(socketFD);
        return 0;
    } else if (strcmp("Ok", responseMsg) != 0) {
        printf("Error: Server is not sending files\n");
        close(socketFD);
        return -1;
    }

    // Request cached timestamp
    if (send(socketFD, "Cached", strlen("Cached"), 0) < 0) {
        perror("Error at socket");
    }
    if (recv(socketFD, timestamp, TIMESTAMP_LENGTH, 0) < 0) {
        perror("Error in receiving timestamp");
    }
    close(socketFD);

    if (index == -1) {
        Insert(filename, timestamp, -1);
        return 0;
    }

    // Check stored timestamp in cache
    int fileDescriptor = open("_cache_/access.dat", O_RDONLY);
    if (fileDescriptor < 0) {
        perror("Error opening file");
        return -1;
    }

    if (lseek(fileDescriptor, 149 * index, SEEK_SET) == -1) {
        perror("Error seeking in file");
        close(fileDescriptor);
        return 0;
    }
    
    // Read from cache
    if (read(fileDescriptor, buff, 149) < 0) {
        perror("Error reading from cache");
        close(fileDescriptor);
        return -1;
    }

    strncpy(storedTime, buff + 128, TIMESTAMP_LENGTH - 1);
    storedTime[TIMESTAMP_LENGTH - 1] = '\0';
    close(fileDescriptor);

    // Compare timestamps
    if (compareTime(storedTime, timestamp) == 0) {
        return 1;
    } else {
        Insert(filename, timestamp, index); // Update cache
        return 0; 
    }
}

void clearCache() {
    if (!clear_cache_flag) {
        return;
    }

    int fileDescriptor = open("_cache_/access.dat", O_RDONLY);
    if (fileDescriptor < 0) {
        perror("Error opening file");
        return;
    }

    char buff[149];
    char file1[128], file2[128], file3[128], file4[128];
    char command[COMMAND_SIZE];

    // Read and store each filename from the cache file
    if (read(fileDescriptor, buff, 149) > 0) {
        strncpy(file1, buff, 128);
    }
    if (read(fileDescriptor, buff, 149) > 0) {
        strncpy(file2, buff, 128);
    }
    if (read(fileDescriptor, buff, 149) > 0) {
        strncpy(file3, buff, 128);
    }
    if (read(fileDescriptor, buff, 149) > 0) {
        strncpy(file4, buff, 128);
    }
    
    close(fileDescriptor);

   
    snprintf(command, sizeof(command),
             "find _cache_ -type f -not -name 'access.dat' -not -name '%s' -not -name '%s' -not -name '%s' -not -name '%s' -delete",
             file1, file2, file3, file4);

        printf("Files deletted\n");
    if (!executeCommand(command)) {
        printf("Failed to execute cache clearing command\n");
    }
}

//--------------------------------------------------------Caching Ends--------------------------------------------------------

// Function to receive data from the server
int receiveFileData(const char *fileName, const char *serverIP, int serverPort, struct user *user,char *temporary_file)
{   char tstemp[20];
    int fileDescriptor = open(temporary_file, O_CREAT | O_RDWR | O_TRUNC, 0644);
    if (fileDescriptor < 0)
    {
        perror("Error opening file for writing");
        return -1; 
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
        return -1; 
    }

    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(serverPort);

    if (inet_pton(AF_INET, serverIP, &serverAddress.sin_addr) <= 0)
    {
        perror("Invalid address/ Address not supported");
        close(fileDescriptor);
        close(socketFD);
        return -1; 
    }

    // Connecting to the server
    if (connect(socketFD, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0)
    {
        perror("Connection Failed");
        close(fileDescriptor);
        close(socketFD);
        return -1; 
    }

    // Sending action request ("Fetch")
    ret = send(socketFD, "Fetch", strlen("Fetch"), 0);
    if (ret == -1)
    {
        perror("Failed to send Fetch request");
        close(fileDescriptor);
        close(socketFD);
        return -1; 
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
        return -1; 
    }

    // Receiving server response ("Ok" or "No file")
    ret = recv(socketFD, responseMsg, sizeof(responseMsg), 0);
    if (ret == -1)
    {
        perror("Failed to receive message");
        close(fileDescriptor);
        close(socketFD);
        return -1; 
    }
    responseMsg[ret] = '\0';

    if (strcmp("No file", responseMsg) == 0)
    {
        printf("File is not present on the server\n");
        close(fileDescriptor);
        close(socketFD);
        return 0;
    }

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
    // ret=send(socketFD,"TimeStemp",strlen("TimeStemp")+2,0);
    // if(ret<0){
    //     perror("Error at timestemp");
    // }
    // ret=recv(socketFD,tstemp,20,0);
    // if(ret<0){
    //     perror("Error at receving timestemp");
    // }
    // printf("\n\nTime: %s\n%d\n",tstemp,ret);
    // Confirming to send the file
    ret = send(socketFD, "Send", strlen("Send"), 0);
    if (ret < 0)
    {
        printf("Error sending confirmation to receive file\n");
        close(fileDescriptor);
        close(socketFD);
        return -1;
    }
    while ((ret = recv(socketFD, buffer, BUFFER_SIZE, 0)) > 0)
    {
        if (write(fileDescriptor, buffer, ret) <= 0)
        {
            perror("Error writing to the file");
            close(fileDescriptor);
            close(socketFD);
            return -1; 
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
int sendFileData(const char *fileName, const char *serverIP, int serverPort, struct user *user,char *temporary_file)
{
    int fileDescriptor = open(temporary_file,O_CREAT | O_RDONLY);
    if (fileDescriptor < 0)
    {
        perror("Error opening file for reading");
        return -1; 
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
        return -1; 
    }

    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(serverPort);

    if (inet_pton(AF_INET, serverIP, &serverAddress.sin_addr) <= 0)
    {
        perror("Invalid address/ Address not supported");
        close(fileDescriptor);
        close(socketFD);
        return -1; 
    }

    // Connecting to the server
    if (connect(socketFD, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0)
    {
        perror("Connection Failed");
        close(fileDescriptor);
        close(socketFD);
        return -1; 
    }

    // Sending action request ("Create/Store")
    ret = send(socketFD, "Create/Store", strlen("Create/Store"), 0);
    if (ret == -1)
    {
        perror("Failed to send Create/Store request");
        close(fileDescriptor);
        close(socketFD);
        return -1; 
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
        return -1; 
    }

    // Receiving server response ("Ok")
    ret = recv(socketFD, responseMsg, sizeof(responseMsg), 0);
    if (ret == -1)
    {
        perror("Failed to receive message");
        close(fileDescriptor);
        close(socketFD);
        return -1; 
    }
    responseMsg[ret]='\0';
    printf("\nREspnse mesg%s\n",responseMsg);
    if (strcmp(responseMsg, "AD") == 0)
    {
        printf("Acess Denied :You don't have permision to write\n");
        close(fileDescriptor);
        close(socketFD);
        return -1; 
    }
    else if(strcmp(responseMsg,"Ok")!=0){
        printf("Cannot Save file in server\n");
        close(fileDescriptor);
        close(socketFD);
        return -1; 
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
            return -1; 
        }
        if (bytesRead < BUFFER_SIZE)
        {
            break;
        }
    }

    char serverResponse[MSG_SIZE];
    ret = recv(socketFD, serverResponse, sizeof(serverResponse), 0);
    if (ret == -1)
    {
        perror("Failed to receive message");
        close(fileDescriptor);
        close(socketFD);
        return -1; 
    }
    serverResponse[ret] = '\0';
    printf("Server response: %s\n", serverResponse);
    close(fileDescriptor);
    close(socketFD);
    return 1; // Success
}

int executeCommand(char *command)
{
    int returnValue = system(command);

    if (returnValue == -1)
    {
        perror("Error executing command");
        return 0; 
    }

    return 1; // Success
}
int rem(char *filename,int flag){
    char delete[FILENAME_MAX];
    strcpy(delete,"rm ");
    strcat(delete,filename);
    return executeCommand(delete);
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
        return -1; 
    }

    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(serverPort);

    if (inet_pton(AF_INET, serverIP, &serverAddress.sin_addr) <= 0)
    {
        perror("Invalid address/ Address not supported");
        close(socketFD);
        return -1; 
    }

    // Connecting to the server
    if (connect(socketFD, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0)
    {
        perror("Connection Failed");
        close(socketFD);
        return -1; 
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
        ret = send(socketFD, filename, strlen(filename), 0);
        if (ret < 0)
        {
            perror("Sending Error \n");
            return 0;
        }
        printf("%s %ld\n",filename,strlen(filename));
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

int changePermission(const char *fileName, const char *serverIP, int serverPort, struct user *user)
{
    char responseMsg[MSG_SIZE];
    int socketFD, ret;
    struct sockaddr_in serverAddress;
    char buffer[BUFFER_SIZE] = {0};

    // Creating a socket
    if ((socketFD = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("Socket creation error");
        return -1; 
    }

    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(serverPort);

    if (inet_pton(AF_INET, serverIP, &serverAddress.sin_addr) <= 0)
    {
        perror("Invalid address/ Address not supported");
        close(socketFD);
        return -1; 
    }

    // Connecting to the server
    if (connect(socketFD, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0)
    {
        perror("Connection Failed");
        close(socketFD);
        return -1; 
    }
    ret = send(socketFD, "ChangePerm", strlen("ChangePerm"), 0);
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
    if (strcmp(responseMsg, "ChangePerm") == 0)
    {
        char updatedName[381];
        strcpy(updatedName, fileName);
        strcat(updatedName, "/");
        strcat(updatedName, user->username);
        printf("%s\n", updatedName);
        ret = send(socketFD, updatedName, sizeof(updatedName), 0);
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
        printf("%s\n", responseMsg);
        if (strcmp(responseMsg, "0") == 0)
        {
            printf("Successfully updated!\n");
            return 0; // file not present
        }
        if (strcmp(responseMsg, "1") == 0)
        {
            printf("You are not the creator of the file, so not able to change the permission\n");
            return 0; // file not present
        }
        if (strcmp(responseMsg, "2") == 0)
        {
            printf("no such file!\n");
            return 0; // file not present
        }
        if (strcmp(responseMsg, "3") == 0)
        {
            printf("wrong input!\n");
            return 0; // file not present
        }
        else
        {
            printf("something went wrong!\n");
        }
    }
    else
    {
        perror("Something wrong in connection\n");
    }
}
int fetch_available_files(const char *serverIP, int serverPort, struct user *user){
    int socketFD, ret;
    struct sockaddr_in serverAddress;
    char buffer[BUFFER_SIZE] = {0};
    char responseMsg[MSG_SIZE];
    char url[150];
    int byteRead;
    strcpy(url,user->username);
    // Creating a socket
    if ((socketFD = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("Socket creation error");
        
        return -1; 
    }

    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(serverPort);

    if (inet_pton(AF_INET, serverIP, &serverAddress.sin_addr) <= 0)
    {
        perror("Invalid address/ Address not supported");
        close(socketFD);
        return -1; 
    }

    // Connecting to the server
    if (connect(socketFD, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0)
    {
        perror("Connection Failed");
        close(socketFD);
        return -1; 
    }
    if(send(socketFD,"AvailableFile",strlen("AvailableFile"),0)<0){
        perror("Error: ");
        close(socketFD);
        return -1;
    }
    if((byteRead=recv(socketFD,responseMsg,MSG_SIZE,0))<0){
        perror("Error : ");
        close(socketFD);
        return -1;
    }
    responseMsg[byteRead]='\0';
    if(strcmp(responseMsg,"Ok")!=0){
        printf("Something wrong with server\n");
        close(socketFD);
        return -1;

    }
    //Creating the request
    strcat(url,"/fileAndPermision");
    if(send(socketFD,url,strlen(url),0)<0){
        perror("Error :");
        close(socketFD);
        return -1;
    }
    int i=0;
    while(1){
        if((byteRead=recv(socketFD,buffer,BUFFER_SIZE,0))<0){
            perror("Error : ");
        }
        buffer[byteRead]='\0';
        if(strcmp(buffer,"END")==0){
            printf("------------------------That's all------------------------");
            close(socketFD);
            return 1;
        }
        else{
            printf("%d)%s\n",i,buffer);
            i++;
            send(socketFD,"Ok",strlen("Ok"),0);
        }

    }return 1;
}
//1 if delete is done
//0 for no delete
//-1 for error
int DeleteFile(char *fileName,const char *serverIP, int serverPort, struct user *user){
char responseMsg[MSG_SIZE];
    int socketFD, ret;
    struct sockaddr_in serverAddress;
    char buffer[BUFFER_SIZE] = {0};
    char response[MSG_SIZE];

    // Creating a socket
    if ((socketFD = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("Socket creation error");
        return -1; 
    }

    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(serverPort);

    if (inet_pton(AF_INET, serverIP, &serverAddress.sin_addr) <= 0)
    {
        perror("Invalid address/ Address not supported");
        close(socketFD);
        return -1; 
    }

    // Connecting to the server
    if (connect(socketFD, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0)
    {
        perror("Connection Failed");
        close(socketFD);
        return -1; 
    }

    ret=send(socketFD,"delete",strlen("delete"),0);
    if(ret<0){
        perror("Error ");
        close(socketFD);
        return -1;
    }
    ret=recv(socketFD,response,sizeof(response),0);
    if(ret<0){
        perror("Error ");
        close(socketFD);
        return -1;
    }
    response[ret]='\0';
    if(strcmp(response,"url")!=0){
        printf("Some problem in url part\n");
        perror("Error ");
        close(socketFD);
        return -1;
    }
    char url[150];
    strcpy(url, user->username);
    strcat(url, "/");
    strcat(url, fileName);
    ret = send(socketFD, url, strlen(url), 0);
    if(ret<0){
        perror("Error ");
        close(socketFD);
        return -1;
    }
    ret=recv(socketFD,response,sizeof(response),0);
    if(ret<0){
        perror("Error ");
        close(socketFD);
        return -1;
    }
    close(socketFD);
    response[ret]='\0';
    if(strcmp(response,"done")==0){
        
        return 1;
    }
    else{
        return 0;
    }
}