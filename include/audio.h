#ifndef AUDIO_H
#define AUDIO_H

#include <stdio.h>
#include <stdint.h>
#include <unistd.h> 
#include <stdlib.h>  
#include <string.h>  
#include <sys/time.h>
#include <sys/socket.h>  

// Sample Rate: 44.1k
// Sample Format: 16-bit
// Channels: 2
// Chunk Duration: 25ms
#define ChunkBytes 680

#define MAX_BUFFER_SIZE 50
#define MAX_OUT_OF_ORDER 10

typedef struct AudioPacket{
    uint32_t PacketNumber;
    uint16_t AudioDataPCM[ChunkBytes];
    uint32_t timestamp_ms;
} AudioPacket;

typedef struct {
    AudioPacket* packets[MAX_BUFFER_SIZE];
    // int head;                    // Next position to write (newest)
    // int tail;                    // Oldest packet position
    int count;                   // Number of packets in buffer
    uint32_t next_expected_seq;  // For rebuilding audio in order
} AudioBuffer;

AudioPacket* create_audio_packet(uint32_t packet_number, const uint16_t* pcm_data,size_t bytes_read);
uint64_t get_timestamp_ms(void);
void init_circular_buffer(AudioBuffer* buffer);
AudioPacket* GetNextPacket(AudioBuffer* buffer);
int ReceiveBufferPacket(int sock_fd,AudioBuffer* buffer);

#endif