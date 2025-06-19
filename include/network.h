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

#include <portaudio.h>


#define PORT "6000"

#define MULTICAST_GROUP "239.0.0.1"
#define MULTICAST_PORT 12345
#define MULTICAST_TTL 1

void SetupSender(int *sock_fd);
void SendData(int *sock_fd, const AudioPacket *packet,size_t packet_size);
void SetupReceiver(const char *ServerIP, int *sock_fd);  
int ReceiveData(int *sock_fd, char *buffer);
void PacketSetupAndSend(FILE *audio_file);
void ReceiveAudio(const char *ServerIP, AudioBuffer *buffer);


#endif