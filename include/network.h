#ifndef NETWORK_H
#define NETWORK_H

#define _POSIX_C_SOURCE 200112L
#define _DEFAULT_SOURCE

#include "audio.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>


#define PORT "6000"

void SetupSender(int *sock_fd);
void SetupReceiver(const char *ServerIP, int *sock_fd);  
void SendData(int *sock_fd, const AudioPacket *packet,size_t packet_size);
int ReceiveData(int *sock_fd, char *buffer);
void PacketSetupAndSend(FILE *audio_file);
void ReceiveAudio(const char *ServerIP);

#endif