#ifndef NETWORK_H
#define NETWORK_H
#include "audio.h"
#define _POSIX_C_SOURCE 200112L

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#define PORT "6000"

void SetupSender(int *sock_fd);
void SetupReceiver(const char *ServerIP, int *sock_fd);  // Changed char* to const char*
void SendData(int *sock_fd, char *message);
void ReceiveData(int *sock_fd, char *buffer);

void send_audio_packet(int sock_fd, const AudioPacket *packet, const struct sockaddr* client_addr, socklen_t addr_len);
int receive_and_buffer_packet(int sock_fd, AudioBuffer *buffer);
AudioPacket *get_next_ordered_packet(AudioBuffer *buffer);

// New function to setup connection and stream audio
void setup_and_stream_audio(FILE *audio_file);

// Add new function for complete audio reception process
void ReceiveAudio(const char *ServerIP, const char *output_filename);

#endif