#include "socket-client.h"

#include <sys/socket.h>
#include <sys/un.h>
#include <sys/types.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

static void socketError(char *msg)
{
    printf("%s\n", msg);
}

void socketClientConnect(int *socketFD, char* socketPath)
{
    *socketFD = socket(AF_UNIX, SOCK_STREAM|SOCK_NONBLOCK, 0);
    
    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, socketPath, sizeof(addr.sun_path)-1);

    int ret = connect(*socketFD, (struct sockaddr*)&addr, sizeof(addr));
    if (ret != 0){
        socketError("Connect failed");
        *socketFD = -1;
    }
}

int socketClientRead(int socketFD, char* buffer, int buffLength)
{
    return recv(socketFD, buffer, buffLength, 0);
}

int socketClientWrite(int socketFD, char* buffer, int buffLength)
{
   return send(socketFD, buffer, buffLength, 0);
}

void socketClientClose(int socketFD)
{
    shutdown(socketFD, 2);
}