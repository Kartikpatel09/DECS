#include "clientHeader.h"
#include "loginHeader.h"

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        printf("Usage: %s <server_ip> <server_port>\n", argv[0]);
        return -1;
    }

    int inputChoice;
    const char *serverIP = argv[1];
    int serverPort = atoi(argv[2]);
    struct user *user = loginPage(serverPort, serverIP);
    char *temporary_file="temp";
    int rec;
    // Clear the temporary file or create it if it doesn't exist
    int fd = open(temporary_file, O_TRUNC | O_CREAT | O_RDWR, 0644);
    if (fd < 0)
    {
        perror("Error initializing temporary file");
        return -1;
    }
    close(fd);

    while (1)
    {
        printf("-------------------------------Options Available--------------------------------\n");
        printf("Enter the task that you want to perform:\n");
        printf("Enter 1 for creating and storing a new file on the server\n");
        printf("Enter 2 for opening file in (R/W) mode \n");
        printf("Enter 3 for opening file (R) mode\n");
        printf("Enter 4 for changing file permission\n");
        printf("Enter 5 for knowing available files: \n");
        printf("Enter 6 if you wanted to send saved file to server: \n");
        printf("Enter 7 for exiting\n");
        printf("Enter your choice: ");
        scanf("%d", &inputChoice);
        char fileName[FILENAME_MAX_LENGTH];
        printf("--------------------------------------------------------------------------------\n");

        switch (inputChoice)
        {
        case 1:

            printf("Enter the name of the file: ");
            scanf("%s", fileName);
            if (checkFilePresence(fileName, serverIP, serverPort) == 1)
            {
                printf("File Already Present enter other name\n");
                break;
            }
            // Open the file in nano editor for editing before uploading
            if (!executeCommand("nano temp"))
            {
                printf("Error opening the file\n");
                return -1;
            }

            // Send the file to the server
            if (sendFileData(fileName, serverIP, serverPort, user,temporary_file) == -1)
            {
                return -1;
            }
            remove("temp");
            break;

        case 2:

            printf("Enter the name of the file: ");
            scanf("%s", fileName);
            if (checkFilePresence(fileName, serverIP, serverPort) == 0)
            {
                printf("File is not present\n");
                break;
            }
            // Fetch the file from the server
        
            if ((rec = receiveFileData(fileName, serverIP, serverPort, user,temporary_file)) == -1)
            {
                return -1;
            }
            if (rec == 0)
                break;

            // Open the fetched file in nano editor for editing
            if (!executeCommand("nano temp"))
            {
                printf("Error opening the file\n");
                return -1;
            }

            // Send the updated file back to the server
            if (sendFileData(fileName, serverIP, serverPort, user,temporary_file) == -1)
            {
                return -1;
            }
            remove("temp");
            break;
        case 3:
            char choice;
            printf("Enter the name of the file: ");
            scanf("%s", fileName);
            if (checkFilePresence(fileName, serverIP, serverPort) == 0)
            {
                printf("File is not present\n");
                break;
            }
            printf("Do you want to store it's copy?(Y/N): \n");
            scanf(" %c",&choice);
            if(choice=='Y'){
                // Fetch the file from the server
              
                if ((rec = receiveFileData(fileName, serverIP, serverPort, user,fileName)) == -1)
                {
                    return -1;
                }
                if (rec == 0)
                    break;
                printf("\nFile has succesfully stored in %s\n",fileName);
            }
            printf("-----------------File content starts File name: %s--------------------------\n",fileName);
          
            if ((rec = receiveFileData(fileName, serverIP, serverPort, user,temporary_file)) == -1)
            {
                return -1;
            }
            if (rec == 0)
                break;
            if (!executeCommand("cat temp"))
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
            if(fetch_available_files(serverIP,serverPort,user)==-1){
                printf("unable to satisfy the request\n");
            }
            break;
        case 6:
            printf("Enter the path of file: ");
            scanf("%s", fileName);
            if (checkFilePresence(fileName, serverIP, serverPort) == 1)
            {
                printf("File Already Present enter other name\n");
                break;
            }
            // Send the file to the server
            if (sendFileData(fileName, serverIP, serverPort, user,fileName) == -1)
            {
                return -1;
            }
            
            break;
        case 7:
            return 0;

        default:
            printf("Incorrect choice\n");

            break;
        }
    }

    return 0;
}
