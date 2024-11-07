#include "headers.h"
#define MAX_CLIENTS 1024
#define BUFFER_SIZE 1024
#define FILENAME_MAX_LENGTH 128
#define COMMAND_SIZE 512
#define MAX_URL_LENGTH 256
#define ACCESS_DATA_SIZE 2333
#define MSG_SIZE 20
#define MAX_THREADS 50
struct user
{
    char username[50];
    char password[50];
};
char *delimiter="_empty_";
typedef struct
{
    char filename[256];
    char creator[50];
    char readUsers[20][50];
    char writeUsers[20][50];
    char numReaders[3];
    char numWriters[3];
    char timestemp[20];
} accessData;
pthread_mutex_t array_lock = PTHREAD_MUTEX_INITIALIZER;
typedef struct
{
    char filename[FILENAME_MAX_LENGTH];
    int reader_count;
    int threads_using;
    pthread_mutex_t lock;
} File_Lock;
File_Lock file_lock[MAX_THREADS];
//------------------------------------LOCK CODE-------------------------------------------------------------------
// function to initialize file lock
void initialize_lock()
{
    for (int i = 0; i < MAX_THREADS; i++)
    {
        strcpy(file_lock[i].filename, "NULL");
        pthread_mutex_init(&file_lock[i].lock, NULL);
        file_lock[i].reader_count = 0;
        file_lock[i].threads_using = 0;
    }
}
void destroy_lock()
{
    for (int i = 0; i < MAX_THREADS; i++)
    {
        pthread_mutex_destroy(&file_lock[i].lock); // Destroy the mutex
    }
}
// return lock to file
File_Lock *getLock(const char *filename)
{

    pthread_mutex_lock(&array_lock);

    for (int i = 0; i < MAX_THREADS; i++)
    {
        if (strcmp(file_lock[i].filename, filename) == 0)
        {
            file_lock[i].threads_using++;
            pthread_mutex_unlock(&array_lock);
            return &file_lock[i];
        }
    }

    for (int i = 0; i < MAX_THREADS; i++)
    {
        if (strcmp(file_lock[i].filename, "NULL") == 0)
        {
            strncpy(file_lock[i].filename, filename, FILENAME_MAX_LENGTH - 1);
            file_lock[i].filename[FILENAME_MAX_LENGTH - 1] = '\0';
            pthread_mutex_unlock(&array_lock);
            return &file_lock[i];
        }
    }

    pthread_mutex_unlock(&array_lock);
    return NULL;
}
//--------------------------------------------------------LOCK CODE END------------------------------------------------
int BashexecuteCommand(char *command)
{
    int returnValue = system(command);

    if (returnValue == -1)
    {
        perror("Error executing command");
        return 0; 
    }

    return 1; // Success
}
int populateAccess(char *filename, char *user, char *command, char *timestemp)
{   int Flag=1;   
    if(access(filename, F_OK)!=0){
        int file_fd = open("_metadata_/access.dat", O_CREAT | O_RDWR);
        if (file_fd < 0)
        {
            perror("Error opening file");
            exit(0);
        }
        
        accessData *acc = (accessData *)malloc(sizeof(accessData));
        
        char buff[ACCESS_DATA_SIZE];
        strcpy(acc->filename, filename);
        strcpy(acc->creator, user);
        strcpy(acc->readUsers[0], user);
        for (int i = 1; i < 20; i++)
        {
            strncpy(acc->readUsers[i], delimiter,50);
        }
        strcpy(acc->writeUsers[0], user);
        for (int i = 1; i < 20; i++)
        {
            strncpy(acc->writeUsers[i], delimiter,50);
        }
        sprintf(acc->numWriters, "%d", 1);
        sprintf(acc->numReaders, "%d", 1);
        time_t curr_time = time(NULL);
        if (curr_time == ((time_t)-1))
        {
            perror("Failed to obtain the current time");
            return 1;
        }
        struct tm *tm_info = localtime(&curr_time);
        strftime(timestemp, 20, "%Y-%m-%d %H:%M:%S", tm_info);
        strncpy(acc->timestemp,timestemp,20);
        int f=0;
        while(read(file_fd,buff,ACCESS_DATA_SIZE)>0){
            if(strncmp(buff,delimiter,256)==0){
                lseek(file_fd,-ACCESS_DATA_SIZE,SEEK_CUR);
                write(file_fd, acc->filename, sizeof(acc->filename));
                write(file_fd, acc->creator, sizeof(acc->creator));
                write(file_fd, acc->readUsers, sizeof(acc->readUsers));
                write(file_fd, acc->writeUsers, sizeof(acc->writeUsers));
                write(file_fd, acc->numReaders, sizeof(acc->numReaders));
                write(file_fd, acc->numWriters, sizeof(acc->numWriters));
                write(file_fd, timestemp, 20);
                write(file_fd, "\n", 1);
                close(file_fd);
                f=1;
                Flag=0;
            }if(f){break;}
        }
        
}

    if (access(filename, F_OK) != 0 && Flag)
    {
        int file_fd = open("_metadata_/access.dat", O_CREAT | O_APPEND | O_RDWR);
        if (file_fd < 0)
        {
            perror("Error opening file");
            exit(0);
        }
        accessData *acc = (accessData *)malloc(sizeof(accessData));
        strcpy(acc->filename, filename);
        strcpy(acc->creator, user);
        strcpy(acc->readUsers[0], user);
        for (int i = 1; i < 20; i++)
        {
            strncpy(acc->readUsers[i], delimiter,50);
        }
        strcpy(acc->writeUsers[0], user);
        for (int i = 1; i < 20; i++)
        {
            strncpy(acc->writeUsers[i], delimiter,50);
        }
        sprintf(acc->numWriters, "%d", 1);
        sprintf(acc->numReaders, "%d", 1);
        write(file_fd, acc->filename, sizeof(acc->filename));
        write(file_fd, acc->creator, sizeof(acc->creator));
        write(file_fd, acc->readUsers, sizeof(acc->readUsers));
        write(file_fd, acc->writeUsers, sizeof(acc->writeUsers));
        write(file_fd, acc->numReaders, sizeof(acc->numReaders));
        write(file_fd, acc->numWriters, sizeof(acc->numWriters));
        time_t curr_time = time(NULL);
        if (curr_time == ((time_t)-1))
        {
            perror("Failed to obtain the current time");
            return 1;
        }
        struct tm *tm_info = localtime(&curr_time);
        strftime(timestemp, 20, "%Y-%m-%d %H:%M:%S", tm_info);
        write(file_fd, timestemp, 20);
        write(file_fd, "\n", 1);
        close(file_fd);
    }
    else if (!strcmp("Fetch", command))
    {
        int flag = 0;
        int file_fd = open("_metadata_/access.dat", O_RDONLY);
        if (file_fd < 0)
        {
            perror("Error opening file");
            exit(0);
        }
        char buffer[ACCESS_DATA_SIZE];
        int readAccess = -1;
        while (read(file_fd, buffer, ACCESS_DATA_SIZE) > 0)
        {
            if (!strcmp(buffer, filename))
            {
                for (int i = 0; i < 20; i++)
                {
                    if (!strcmp(&buffer[306 + (i * 50)], user))
                    {
                        strncpy(timestemp, buffer + (ACCESS_DATA_SIZE - 21), 20);
                        close(file_fd);
                        readAccess = 0;
                        flag = 1;
                        break;
                    }
                }
            }
            if (flag)
            {
                break;
            }
        }
        return readAccess;
    }
    else if (strcmp(command, "Create/Store") == 0)
    {
        int flag = 0;
        int file_fd = open("_metadata_/access.dat", O_RDWR);
        if (file_fd < 0)
        {
            perror("Error opening file");
            exit(0);
        }
        char buffer[ACCESS_DATA_SIZE];
        int writeAccess = -1;
        while (read(file_fd, buffer, ACCESS_DATA_SIZE) > 0)
        {
            if (!strcmp(buffer, filename))
            {
                for (int i = 0; i < 20; i++)
                {
                    if (!strcmp(&buffer[1306 + (i * 50)], user))
                    {
                        strncpy(buffer + (ACCESS_DATA_SIZE - 21), timestemp, 20);

                        lseek(file_fd, -ACCESS_DATA_SIZE, SEEK_CUR);
                        write(file_fd, buffer, ACCESS_DATA_SIZE);
                        close(file_fd);
                        writeAccess = 0;
                        flag = 1;
                        break;
                    }
                }
            }
            if (flag)
            {
                break;
            }
        }
        return writeAccess;
    }
}
// Function to send file data to the client
int sendFileData(const char *filename, int connection_fd)
{

    File_Lock *my_file_lock = getLock(filename);
    if (my_file_lock == NULL)
    {
        fprintf(stderr, "No available lock for the file: %s\n", filename);
        return -1;
    }

    int file_fd = open(filename, O_RDONLY);
    if (file_fd < 0)
    {
        perror("Error opening file");

        return -1;
    }

    char buffer[BUFFER_SIZE];
    int bytesRead, bytesSent;

    pthread_mutex_lock(&(my_file_lock->lock));
    my_file_lock->reader_count += 1;
    pthread_mutex_unlock(&(my_file_lock->lock));

    while ((bytesRead = read(file_fd, buffer, BUFFER_SIZE)) > 0)
    {
        bytesSent = send(connection_fd, buffer, bytesRead, 0);
        if (bytesSent < 0)
        {
            perror("Error in sending data");
            close(file_fd);
            return -1;
        }
    }

    pthread_mutex_lock(&(my_file_lock->lock));
    my_file_lock->threads_using--;
    my_file_lock->reader_count--;

    if (my_file_lock->threads_using == 0)
    {
        strcpy(my_file_lock->filename, "NULL");
    }

    pthread_mutex_unlock(&(my_file_lock->lock));

    printf("File request satisfied\n");
    close(file_fd);

    return 1;
}
// Function to receive file data from the client
int receiveFileData(const char *filename, int connection_fd)
{
    File_Lock *my_file_lock = getLock(filename);
    if (my_file_lock == NULL)
    {
        fprintf(stderr, "No available lock for the file: %s\n", filename);
        return -1;
    }
    int flag = 0;
    int file_fd = open(filename, O_CREAT | O_TRUNC | O_RDWR, 0644);
    if (file_fd < 0)
    {
        perror("Error creating file");
        return -1;
    }

    char buffer[BUFFER_SIZE];
    int bytesReceived;
    while (my_file_lock->reader_count != 0)
        ;
    pthread_mutex_lock(&(my_file_lock->lock));
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
    my_file_lock->threads_using--;
    if (my_file_lock->threads_using == 0)
    {
        flag = 1;
    }
    if (flag)
    {
        strcpy(my_file_lock->filename, "NULL");
    }
    pthread_mutex_unlock(&(my_file_lock->lock));
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
    char url[MAX_URL_LENGTH];
    if (strcmp("Fetch", command) == 0)
    {
        result = recv(connection_fd, url, FILENAME_MAX_LENGTH, 0);
        if (result <= 0)
        {
            printf("Error in receiving filename\n");
            return -1;
        }
        url[result] = '\0';
        char *user = strtok(url, "/");
        char *filename = strtok(NULL, "/");
        if (access(filename, F_OK) != 0)
        {
            send(connection_fd, "No file", strlen("No file"), 0);
            return -1;
        }
        else
        {
            char tstemp[20];
            int readAccess = populateAccess(filename, user, command, tstemp);

            if (readAccess == -1)
            {
                int result = send(connection_fd, "No Read access", strlen("No Read access"), 0);
                if (result <= 0)
                {
                    printf("Error sending read access\n");
                    return -1;
                }
            }
            else
            {
                char request[MSG_SIZE];
                send(connection_fd, "Ok", strlen("Ok"), 0);
                // result = recv(connection_fd, request, sizeof(request), 0);
                // if(result<0){
                //     perror("Error at timestep: ");
                // }
                // request[result]='\0';
                // if(strcmp(request,"TimeStemp")==0){
                //     result=send(connection_fd,tstemp,20,0);
                //     printf("Value of result after sending %s\n",(tstemp));
                // }
                // else{exit(0);}
                result = recv(connection_fd, request, sizeof(request), 0);
                request[result] = '\0';
                if (result < 0)
                {
                    return -1;
                }
                if (strcmp(request, "Send") == 0)
                {
                    sendFileData(filename, connection_fd);
                }
                else if (strcmp(request, "Cached") == 0)
                {
                    result = send(connection_fd, tstemp, 20, 0);
                    if (result < 0)
                    {
                        perror("Error at timestemp sending");
                    }
                    close(connection_fd);
                }
                else
                {
                    printf("Unexpected request received: %s and res: %d\n", command, result);
                    return -1;
                }
            }
        }
    }
    else if (strcmp("Create/Store", command) == 0)
    {
        char tstemp[20];
        result = recv(connection_fd, url, MAX_URL_LENGTH, 0);
        if (result <= 0)
        {
            printf("Error in receiving filename\n");
            return -1;
        }
        url[result] = '\0';
        char *user = strtok(url, "/");
        char *filename = strtok(NULL, "/");
        time_t curr_time = time(NULL);
        if (curr_time == ((time_t)-1))
        {
            perror("Failed to obtain the current time");
            return 1;
        }
        struct tm *tm_info = localtime(&curr_time); 
        strftime(tstemp, 20, "%Y-%m-%d %H:%M:%S", tm_info);
        int writeAcess = populateAccess(filename, user, command, tstemp);
        printf("WriteAcess value:%d\n", writeAcess);
        if (writeAcess == -1)
        {
            send(connection_fd, "AD", strlen("AD"), 0);
        }
        else
        {
            send(connection_fd, "Ok", strlen("Ok"), 0);
            receiveFileData(filename, connection_fd);
        }
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
    int file_fd = open("_metadata_/login.dat", O_RDWR | O_APPEND | O_CREAT);
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
    printf("user :%s\n", user->username);
    printf("pwd :%s\n", user->password);
    printf("File request satisfied\n");
    close(file_fd);
}

void checkLogin(struct user *user, int connection_fd)
{
    int file_fd = open("_metadata_/login.dat", O_RDONLY);
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

void checkFileUser(char *fileUser, int connection_fd)
{
    int file_fd = open("_metadata_/access.dat", O_RDWR);
    if (file_fd < 0)
    {
        perror("Error opening file");
        exit(0);
    }

    char buffer[ACCESS_DATA_SIZE];
    int bytesRead, found = 0;
    char fileUserSep[5][256];
    // fileUserSep[0] - file name
    // fileUserSep[1] - which permission
    // fileUserSep[2] - invoke or revoke
    // fileUserSep[3] - affected user
    // fileUserSep[4] - requesting user
    char *tok = strtok(fileUser, "/");
    int i = 0;
    while (tok != NULL)
    {
        strcpy(fileUserSep[i], tok);
        i++;
        tok = strtok(NULL, "/");
    }

    // response 1 - user is not creator
    // response 3 - wrong read, write
    while ((bytesRead = read(file_fd, buffer, ACCESS_DATA_SIZE)) > 0)
    {
        if (strcmp(buffer, fileUserSep[0]) == 0)
        {
            if (strcmp(&buffer[256], fileUserSep[4]))
            {
                int result = send(connection_fd, "1", strlen("1"), 0);
                if (result < 0)
                {
                    perror("Error in sending ");
                }
                close(file_fd);
                return;
            }
            else
            {
                lseek(file_fd, -ACCESS_DATA_SIZE, SEEK_CUR);
                int numReaders = atoi(&buffer[2306]);
                int numWriters = atoi(&buffer[2309]);
                accessData acc;

                if (!strcmp(fileUserSep[1], "read") && (!strcmp(fileUserSep[2], "invoke") || !strcmp(fileUserSep[2], "revoke")))
                {
                    if (!strcmp(fileUserSep[2], "invoke"))
                    {   int flag=0;
                        for (int i = 1; i < 20; i++)
                        {
                            if (strcmp(buffer + 306 + i * 50, delimiter) == 0)
                            {   
                                strncpy(buffer+306+i*50,fileUserSep[3],50);
                                numReaders++;
                                char numReaderStr[3];
                                sprintf(numReaderStr, "%d", numReaders);
                                strncpy(buffer+2306,numReaderStr,3);
                                if(write(file_fd,buffer,ACCESS_DATA_SIZE)<0){
                                    perror("Error in setting permision: ");
                                }

                               flag=1;
                            }if(flag){break;}
                        }
                    }
                    else
                    {
                        printf("revoking read\n");
                        int flag=0;
                        for (int i = 0; i < 20; i++)
                        {
                            if (!strcmp(&buffer[306 + (i * 50)], fileUserSep[3]))
                            {
                                strncpy(buffer+306+i*50,delimiter,50);
                                numReaders--;
                                char numReaderStr[3];
                                sprintf(numReaderStr, "%d", numReaders);
                                strncpy(buffer+2306,numReaderStr,3);
                                if(write(file_fd,buffer,ACCESS_DATA_SIZE)<0){
                                    perror("Error in setting permision: ");
                                }
                                flag=1;
                               
                            }if(flag){break;}
                        }
                    }
                }
                else if (!strcmp(fileUserSep[1], "write") && (!strcmp(fileUserSep[2], "invoke") || !strcmp(fileUserSep[2], "revoke")))
                {
                    if (!strcmp(fileUserSep[2], "invoke"))
                    {
                        int flag=0;
                        for (int i = 1; i < 20; i++)
                        {
                            if (strcmp(buffer + 1306 + i * 50, delimiter) == 0)
                            {   
                                strncpy(buffer+1306+i*50,fileUserSep[3],50);
                                numWriters++;
                                char numWriterStr[3];
                                sprintf(numWriterStr, "%d", numWriters);
                                strncpy(buffer+2306,numWriterStr,3);
                                if(write(file_fd,buffer,ACCESS_DATA_SIZE)<0){
                                    perror("Error in setting permision: ");
                                }

                               flag=1;
                            }if(flag){break;}
                        }
                    }
                    else
                    {   int flag=0;
                        for (int i = 1; i < 20; i++)
                        {
                            if (strcmp(buffer + 306 + i * 50, fileUserSep[3]) == 0)
                            {   
                                strncpy(buffer+306+i*50,delimiter,50);
                                numWriters--;
                                char numWriterStr[3];
                                sprintf(numWriterStr, "%d", numWriters);
                                strncpy(buffer+2306,numWriterStr,3);
                                if(write(file_fd,buffer,ACCESS_DATA_SIZE)<0){
                                    perror("Error in setting permision: ");
                                }

                               flag=1;
                            }if(flag){break;}
                        }
                    }
                }
                else
                {
                    int result = send(connection_fd, "3", strlen("3"), 0);
                    if (result < 0)
                    {
                        perror("Error in sending ");
                    }
                    close(file_fd);
                    return;
                }
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
    else
    {
        int result = send(connection_fd, "0", strlen("0"), 0);
        if (result < 0)
        {
            perror("Error in sending ");
        }
    }
    printf("File request satisfied\n");
    close(file_fd);
}
void listfile(int connection_fd)
{
    char url[150];
    int byteRead;
    int readed;
    char response[MSG_SIZE];
    if ((byteRead = recv(connection_fd, url, sizeof(url), 0)) < 0)
    {
        perror("Error:");
        close(connection_fd);
        return;
    }
    char *userName = strtok(url, "/");
    char storedFile[FILENAME_MAX_LENGTH];
    char buff[ACCESS_DATA_SIZE];

    int file_fd = open("_metadata_/access.dat", O_RDONLY);
    if (file_fd < 0)
    {
        printf("File doesn't exit\n");
    }

    while (read(file_fd, buff, ACCESS_DATA_SIZE) > 0)
    {
        char fileAndPermission[54];
        fileAndPermission[0] = '\0';
        readed = 306;
        int flag = 1;
        for (int i = 0; i < 20; i++)
        {
            if (!strcmp(buff + readed, userName))
            {
                strcpy(fileAndPermission, buff);
                strcat(fileAndPermission, "\t(R)");
                flag = 0;
                break;
            }
            readed += 50;
        }

        readed = 1306;
        for (int i = 0; i < 20; i++)
        {
            if (!strcmp(buff + readed, userName))
            {
                if (flag)
                {
                    strcpy(fileAndPermission, buff);
                }
                strcat(fileAndPermission, "\t(W)");
                break;
            }
            readed += 50;
        }
        if (strlen(fileAndPermission) != 0)
        {
            if (send(connection_fd, fileAndPermission, sizeof(fileAndPermission), 0) < 0)
            {
                perror("Error:");
                close(connection_fd);
                return;
            }
            if ((byteRead = recv(connection_fd, response, MSG_SIZE, 0)) < 0)
            {
                perror("Error : ");
                close(connection_fd);
                return;
            }
            response[byteRead] = '\0';
            if (strcmp(response, "Ok") != 0)
            {
                close(connection_fd);
                return;
            }
        }
    }
    if (send(connection_fd, "END", sizeof("END"), 0) < 0)
    {
        perror("Error:");
        close(connection_fd);
        return;
    }
    close(connection_fd);
}
int is_file_on_server(char *filename)
{   int pos=-1;
    int file_fd = open("_metadata_/access.dat", O_RDONLY);
    char buff[ACCESS_DATA_SIZE];
    if (file_fd < 0)
    {
        printf("File doesn't exit\n");
        return pos;
    }

    while (read(file_fd, buff, ACCESS_DATA_SIZE) > 0)
    {   pos++;
        if (strncmp(buff, filename,256) == 0)
        {
            return pos;
        }
    }
    return pos;
}
int delet(char* URL){
    //-1 file doesn't exist or error
    //0 you don't have permision
    char url[150];
    char buff[ACCESS_DATA_SIZE];
    strcpy(url,URL);
    char* username=strtok(url,"/");
    char *filename=strtok(NULL,"");
    int index=is_file_on_server(filename);
    if(index==-1){
        return -1;
    }
    int file_descriptor=open("_metadata_/access.dat",O_RDWR);
    lseek(file_descriptor,ACCESS_DATA_SIZE*index,SEEK_SET);
    if(read(file_descriptor,buff,ACCESS_DATA_SIZE)<0){
        perror("Error at ");
        close(file_descriptor);
        return 0;
    }
    if(strncmp(buff+256,username,50)!=0){
        return 0;
    }
    strncpy(buff,delimiter,256);
    lseek(file_descriptor,ACCESS_DATA_SIZE*index,SEEK_SET);
    if(write(file_descriptor,buff,ACCESS_DATA_SIZE)<0){
        perror("Error at ");
        close(file_descriptor);
        return 0;
    }
    char cmd[100];
    snprintf(cmd,sizeof(cmd),"rm %s",filename);
    BashexecuteCommand(cmd);
    close(file_descriptor);
    return 1;

}