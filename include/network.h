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
void SendData(int *sock_fd, char *message);
void ReceiveData(int *sock_fd, char *buffer);
// Add these new function prototypes:

void SendBinaryData(int *sock_fd, const uint8_t *data, size_t length);
ssize_t ReceiveBinaryData(int *sock_fd, uint8_t *buffer, size_t max_length);

#endif