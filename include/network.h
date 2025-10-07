#ifndef NETWORK_H
#define NETWORK_H

#define _POSIX_C_SOURCE 200112L
#define _DEFAULT_SOURCE

#include "common.h"
#include "audio.h"
#include "ntp.h"

// Define multicast constants for macOS
#ifndef IP_MULTICAST_TTL
#define IP_MULTICAST_TTL 10
#endif

#ifndef IP_ADD_MEMBERSHIP
#define IP_ADD_MEMBERSHIP 12
#endif

#define PORT "6000"
#define MULTICAST_GROUP "239.0.0.1"
#define MULTICAST_PORT 12345
#define MULTICAST_TTL 1

// Function declarations
void SetupSender(int *sock_fd);
void SendData(int *sock_fd, const AudioPacket *packet, size_t packet_size, SyncPacket *sync_packet);
void SetupReceiver(int *sock_fd);
void PacketSetupAndSend(FILE *audio_file);
int ReceiveBufferPacket(int sock_fd, AudioBuffer *buffer);
void ReceiveAudio(AudioBuffer *buffer);

#endif // NETWORK_H