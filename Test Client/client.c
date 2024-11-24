#include "clientHeader.h"
#include "loginHeader.h"
void handler(int sig)
{
    executeCommand("rm -r _cache_");
    exit(0);
}
int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        printf("Usage: %s <server_ip> <server_port>\n", argv[0]);
        return -1;
    }

    const char *serverIP = argv[1];
    int serverPort = atoi(argv[2]);
    struct user user;
    strcpy(user.username, "kartik");

    if (fetch_available_files(serverIP, serverPort, &user) == -1)
    {
        printf("unable to satisfy the request\n");
    }

    return 0;
}
