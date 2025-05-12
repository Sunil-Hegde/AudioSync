#ifndef NETWORK_H
#define NETWORK_H

#define _POSIX_C_SOURCE 200112L

#include<stdio.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netdb.h>
#include<string.h>
#include<stdlib.h>
#include<arpa/inet.h>
#include<netinet/in.h>

#define PORT "6000"

void SetupSender(int *sock_fd);
void SetupReceiver(char *ServerIP, int *sock_fd);
void SendData(int *sock_fd, int *new_sock_fd, char *message);
void ReceiveData(int *sock_fd, char *buffer);

#endif