#ifndef NETWORK_H
#define NETWORK_H

#include "common.h"
#include "audio.h"
#include "ntp.h"

#ifdef _WINDOWS
    typedef SOCKET socket_t;
#else
    typedef int socket_t;
#endif

#define PORT "6000"
#define MULTICAST_GROUP "239.0.0.1"
#define MULTICAST_PORT 12345
#define MULTICAST_TTL 1

// Function declarations
void SetupSender(socket_t *sock_fd);
void SendData(socket_t *sock_fd, const AudioPacket *packet, size_t packet_size, SyncPacket *sync_packet);
void SetupReceiver(socket_t *sock_fd);
void PacketSetupAndSend(FILE *audio_file);
int ReceiveBufferPacket(socket_t sock_fd, AudioBuffer *buffer);
void ReceiveAudio(AudioBuffer *buffer);

#endif // NETWORK_H