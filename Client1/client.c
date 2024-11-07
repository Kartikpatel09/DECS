#include "clientHeader.h"
#include "loginHeader.h"

int main(int argc, char *argv[])
{   
    if (argc != 3)
    {
        printf("Usage: %s <server_ip> <server_port>\n", argv[0]);
        return -1;
    }
    char *dir = "_cache_/";
    int inputChoice;
    const char *serverIP = argv[1];
    int serverPort = atoi(argv[2]);
    struct user *user = loginPage(serverPort, serverIP);
    int rec;
    executeCommand("rm _cache_/*");
    executeCommand("touch _cache_/access.dat");

    while (1)
    {
        char temporary_file[FILENAME_MAX_LENGTH];
        char nano[COMMAND_SIZE];
        char cat[COMMAND_SIZE];
        int cachingFalg = 0;
        strcpy(temporary_file, dir);

        printf("-------------------------------Options Available--------------------------------\n");
        printf("Enter the task that you want to perform:\n");
        printf("Enter 1 for creating and storing a new file on the server\n");
        printf("Enter 2 for opening file in (R/W) mode \n");
        printf("Enter 3 for opening file (R) mode\n");
        printf("Enter 4 for changing file permission\n");
        printf("Enter 5 for knowing available files: \n");
        printf("Enter 6 if you wanted to send saved file to server: \n");
        printf("Enter 7 for deleting file\n");
        printf("Enter 8 for renaming the file\n");
        printf("Enter 9 for exiting\n");
        printf("Enter your choice: ");
        scanf("%d", &inputChoice);
        char fileName[FILENAME_MAX_LENGTH];
        printf("--------------------------------------------------------------------------------\n");

        switch (inputChoice)
        {
        case 1:

            printf("Enter the name of the file: ");
            scanf("%s", fileName);
            if (checkFilePresence(fileName, serverIP, serverPort,user) == 1)
            {
                printf("File Already Present enter other name\n");
                break;
            }
            strcat(temporary_file, fileName);
            snprintf(nano, sizeof(nano), "nano %s", temporary_file);

            // Open the file in nano editor for editing before uploading
            if (!executeCommand(nano))
            {
                printf("Error opening the file\n");
                return -1;
            }

            // Send the file to the server
            if (sendFileData(fileName, serverIP, serverPort, user, temporary_file,"create") == -1)
            {
                return -1;
            }
            if (!rem(temporary_file, 0))
            {
                printf("Error in deleting file\n");
            }

            break;

        case 2:

            printf("Enter the name of the file: ");
            scanf("%s", fileName);
            if (checkFilePresence(fileName, serverIP, serverPort,user) == 0)
            {
                printf("File is not present\n");
                break;
            }
            strcat(temporary_file, fileName);
            snprintf(nano, sizeof(nano), "nano %s", temporary_file);
            cachingFalg = isCached(fileName, serverIP, serverPort, user);
            if(cachingFalg){printf("Using cached data\n");}
            if (!cachingFalg)
            {   
                // Fetch the file from the server
                if ((rec = receiveFileData(fileName, serverIP, serverPort, user, temporary_file)) == -1)
                {
                    return -1;
                }
                if (rec == 0)
                    break;
            }
            // Open the fetched file in nano editor for editing
            if (!executeCommand(nano))
            {
                printf("Error opening the file\n");
                return -1;
            }

            // Send the updated file back to the server
            if (sendFileData(fileName, serverIP, serverPort, user, temporary_file,"store") == -1)
            {
                return -1;
            }

            break;
        case 3:
            char choice;
            printf("Enter the name of the file: ");
            scanf("%s", fileName);
            if (checkFilePresence(fileName, serverIP, serverPort,user) == 0)
            {
                printf("File is not present\n");
                break;
            }
            strcat(temporary_file, fileName);
            snprintf(nano, sizeof(nano), "nano %s", temporary_file);
            printf("Do you want to store it's copy?(Y/N): ");
            scanf(" %c", &choice);
            cachingFalg = isCached(fileName, serverIP, serverPort, user);
            if (cachingFalg)
            {
                printf("Using cached data\n");
            }
            if (choice == 'Y')
            {
                if (cachingFalg)
                {
                    int firstfd = open(temporary_file, O_RDWR);
                    int secondfd = open(fileName, O_RDWR);
                    int readByte;
                    char buff[1024];
                    while ((readByte = read(firstfd, buff, 1024)) > 0)
                    {
                        write(secondfd, buff, readByte);
                    }
                }
                else
                {
                    // Fetch the file from the server1
                    if ((rec = receiveFileData(fileName, serverIP, serverPort, user, fileName)) == -1)
                    {
                        return -1;
                    }
                    if (rec == 0)
                    {
                        perror("Error at receiving:");
                        exit(1);
                    }
                }
                printf("\nFile has succesfully stored in %s\n", fileName);
            }

            printf("-----------------File content starts File name: %s--------------------------\n", fileName);
            if (!cachingFalg)
            {
                if ((rec = receiveFileData(fileName, serverIP, serverPort, user, temporary_file)) == -1)
                {
                    return -1;
                }
                if (rec == 0)
                {
                    perror("Error at receiving:");
                    exit(1);
                }
            }
            snprintf(cat, sizeof(cat), "cat %s", temporary_file);
            if (!executeCommand(cat))
            {
                printf("Error opening the file\n");
                return -1;
            }
            printf("------------------------------File content over------------------------------\n");

            break;
        case 4:
            printf("Enter the name of the file: ");
            scanf("%s", fileName);
            printf("\n");
            char perm[10];
            printf("Enter which permission needs to be chaged, read or write? :");
            scanf("%s", perm);
            printf("\n");
            char todo[10];
            printf("Enter whether invoke or revoke access : ");
            scanf("%s", todo);
            printf("\n");
            char userName[50];
            printf("Enter the username of the affected user :");
            scanf("%s", userName);
            printf("\n");

            char updatedName[330];
            strcpy(updatedName, fileName);
            strcat(updatedName, "/");
            strcat(updatedName, perm);
            strcat(updatedName, "/");
            strcat(updatedName, todo);
            strcat(updatedName, "/");
            strcat(updatedName, userName);
            printf("%s\n", updatedName);
            if ((changePermission(updatedName, serverIP, serverPort, user) == -1))
            {
                return -1;
            }
            break;

        case 5:
            if (fetch_available_files(serverIP, serverPort, user) == -1)
            {
                printf("unable to satisfy the request\n");
            }
            break;
        case 6:
            printf("Enter the path of file: ");
            scanf("%s", fileName);
            if (checkFilePresence(fileName, serverIP, serverPort,user) == 1)
            {
                printf("File Already Present enter other name\n");
                break;
            }
            // Send the file to the server
            if (sendFileData(fileName, serverIP, serverPort, user, fileName,"create") == -1)
            {
                return -1;
            }

            break;
        case 7:
            printf("Enter the name of the file: ");
            scanf("%s", fileName);
            if (checkFilePresence(fileName, serverIP, serverPort,user) == 0)
            {
                printf("File is not present\n");
                break;
            }
            if(DeleteFile(fileName,serverIP,serverPort,user)==1){
                printf("%s file is deleted\n",fileName);
            }
            else{
                printf("You are not owner you cannot delete file\n");
            }
            break;
        case 8:
            char oldname[FILENAME_MAX_LENGTH];
            char newname[FILENAME_MAX_LENGTH];
            printf("Enter old name of file: ");
            scanf("%s",oldname);
            printf("Enter new name of file: ");
            scanf("%s",newname);
            changeFileName(user,oldname,newname,serverIP,serverPort);
            break;
        case 9:
            return 0;

        default:
            printf("Incorrect choice\n");

            break;
        }
        clearCache();
    }

    return 0;
}
