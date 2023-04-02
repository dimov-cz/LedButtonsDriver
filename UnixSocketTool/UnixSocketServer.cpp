/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   socket.c
 * Author: martin
 * 
 * Created on 27. září 2019, 20:19
 */

#include "UnixSocketServer.h"




void UnixSocketServer::socketError(const char *msg)
{
    printf("%s\n", msg);
}

int UnixSocketServer::Open(const char* socketPath)
{
    this->sockFD = socket(AF_UNIX, SOCK_STREAM|SOCK_NONBLOCK, 0);
    
    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, socketPath, sizeof(addr.sun_path)-1);
    unlink(socketPath);
    int ret = bind(sockFD, (struct sockaddr*)&addr, sizeof(addr));
    if (ret != 0){
        socketError("Bind failed");
        return -1;
    }
    
    if (listen(this->sockFD, MAX_CLIENTS) == -1) {
        socketError("unable to bind socket");
        return -1;
    }
    chmod(socketPath, 00666);
    return this->sockFD;
}


void UnixSocketServer::RemoveClient(int clientFD)
{        
    int indexOfClosingOne = -1;
    for(int i=0; i<this->clientsConnected; i++)
    {
        if (this->clientFDs[i] == clientFD)
        {
            indexOfClosingOne = i;
        }
        if (indexOfClosingOne>=0)
        {
            this->clientFDs[i] = this->clientFDs[i+1];
        }
    }
    this->clientFDs[this->clientsConnected] = 0;
    this->clientsConnected--;
}

/**
 * 
 * @param receiveBuffer
 * @param bufferSize
 * @param inviteMessage if not null we send this as greetings for new connections
 * @return 
 */
int UnixSocketServer::Loop(char* receiveBuffer, int bufferSize, const char* inviteMessage)
{   
    struct timeval tv;
    int maxFD;
    fd_set fds;
    int eventCount;
    
    tv.tv_sec = 0;
    tv.tv_usec = 100;
    
    //printf("clients %i/%i\n", clientsConnected, MAX_CLIENTS);
    if (MAX_CLIENTS>this->clientsConnected)
    {
        FD_ZERO(&fds);
        FD_SET(this->sockFD, &fds);
        maxFD = this->sockFD+1;
        
        eventCount = select(maxFD+1, &fds, NULL, NULL, &tv);
        if (eventCount>0){
            int newClienFD = accept(sockFD, NULL, NULL);
            if (newClienFD != -1){
                this->clientsConnected++;
                this->clientFDs[this->clientsConnected-1] = newClienFD;
                printf("new client FD:%i\n", newClienFD);
                if (inviteMessage != NULL){
                    send(newClienFD, inviteMessage, strlen(inviteMessage), MSG_NOSIGNAL);
                }
            }
        }
    }
    
    FD_ZERO(&fds);
    maxFD = 0;
    for(int i=0; i<this->clientsConnected; i++)
    {
        //printf("Watching %i", clientFDs[i]);
        FD_SET(this->clientFDs[i], &fds);
        if (this->clientFDs[i]>maxFD){
            maxFD = this->clientFDs[i];
        }
    }
    
    eventCount = select(maxFD+1, &fds, NULL, NULL, &tv);
    if (eventCount){
        for(int i=0; i<this->clientsConnected; i++)
        {
            int clientFD = this->clientFDs[i];
            if (FD_ISSET(clientFD, &fds))
            {
                int readret = recv(clientFD, receiveBuffer, bufferSize, 0);
                if (readret>0)
                {
                    return readret;
                }
                else if (readret==-1)
                {
                    if (errno == EWOULDBLOCK){
                        //no data??
                        printf("E would block?\n");
                    }else if (errno == ENOTSOCK){
                        printf("not a socket %i for recv\n", clientFD);
                    }else{
                        //socketRemoveClient(eventFD);
                        printf("??? %i\n", errno);
                    }
                }
                else
                {//==0 => exit
                    printf("Client exit %i\n", clientFD);
                    this->RemoveClient(clientFD);
                    close(clientFD);
                }
            }
        }
    }
    return 0;
}

/**
 * Close, but this dowsnt not delete the socket file.
 * 
 */
void UnixSocketServer::Close()
{
    for(int i=0; i<this->clientsConnected; i++){
        close(this->clientFDs[i]);
    }
    shutdown(this->sockFD, 2);
}

void UnixSocketServer::SendToAll(char *msg)
{
    for(int i=0; i<this->clientsConnected; i++){
        send(this->clientFDs[i], msg, strlen(msg), MSG_NOSIGNAL);
    }
}