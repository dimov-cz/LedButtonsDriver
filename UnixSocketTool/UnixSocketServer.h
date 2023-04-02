/* 
 * File:   socket-server.h
 * Author: martin
 *
 * Created on 13. října 2019, 17:41
 */

#ifndef SOCKET_SERVER_H
#define SOCKET_SERVER_H

#include <sys/socket.h>
#include <sys/un.h>
#include <sys/types.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>


#define MAX_CLIENTS 5



class UnixSocketServer {
public:
    int Open(const char* socketPath);
    int Loop(char* receiveBuffer, int bufferSize, const char* inviteMessage);
    void Close();
    void SendToAll(char *msg); 
protected:
    int sockFD;
    int clientsConnected = 0;
    int clientFDs[MAX_CLIENTS];

    void RemoveClient(int clientFD);
    void socketError(const char *msg);
};

   


#endif /* SOCKET_SERVER_H */

