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

// Define multicast constants for macOS
#ifndef IP_MULTICAST_TTL
#define IP_MULTICAST_TTL 10
#endif

#ifndef IP_ADD_MEMBERSHIP
#define IP_ADD_MEMBERSHIP 12
#endif

// Define struct ip_mreq if not available
#ifndef _STRUCT_IP_MREQ
#define _STRUCT_IP_MREQ
struct ip_mreq
{
    struct in_addr imr_multiaddr;
    struct in_addr imr_interface;
};
#endif

#define PORT "6000"
#define MULTICAST_GROUP "239.0.0.1"
#define MULTICAST_PORT 12345
#define MULTICAST_TTL 1

// Function declarations
void SetupSender(int *sock_fd);
void SendData(int *sock_fd, const AudioPacket *packet, size_t packet_size);
void SetupReceiver(const char *ServerIP, int *sock_fd);
void PacketSetupAndSend(FILE *audio_file);
void ReceiveAudio(const char *ServerIP, AudioBuffer *buffer);

#endif // NETWORK_H