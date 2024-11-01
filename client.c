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

    // Clear the temporary file or create it if it doesn't exist
    int fd = open("temp", O_TRUNC | O_CREAT | O_RDWR, 0644);
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
        printf("Enter 1 for creating and storing a file on the server\n");
        printf("Enter 2 for reading and writing an existing file on the server\n");
        printf("Enter 3 for exiting\n");
        printf("Enter your choice: ");
        scanf("%d", &inputChoice);
        char fileName[FILENAME_MAX_LENGTH];
        printf("++++++++++++++++++++++++++++++Enter file Name+++++++++++++++++++++++++++++++++\n");

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
            if (sendFileData(fileName, serverIP, serverPort, user) == -1)
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
            int rec;
            if ((rec = receiveFileData(fileName, serverIP, serverPort, user)) == -1)
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
            if (sendFileData(fileName, serverIP, serverPort, user) == -1)
            {
                return -1;
            }
            remove("temp");
            break;

        case 3:
            return 0;

        default:
            printf("Incorrect choice\n");

            break;
        }
    }

    return 0;
}
