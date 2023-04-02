/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   socket-client.h
 * Author: martin
 *
 * Created on 6. října 2019, 14:26
 */

#ifndef SOCKET_CLIENT_H
#define SOCKET_CLIENT_H

#ifdef __cplusplus
extern "C" {
#endif

void socketClientConnect(int *socketFD, char* socketPath);
void socketClientClose(int socketFD);
int socketClientRead(int socketFD, char* buffer, int buffLength);
int socketClientWrite(int socketFD, char* buffer, int buffLength);


#ifdef __cplusplus
}
#endif

#endif /* SOCKET_CLIENT_H */

