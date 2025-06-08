#ifndef AUDIO_H
#define AUDIO_H

#include <stdio.h>
#include <stdint.h>
#include <unistd.h> 
#include <stdlib.h>  
#include <string.h>  

#include <sys/time.h>
#include <sys/socket.h>

#include <portaudio.h>

#define SAMPLE_RATE 44100
// Sample Format: 16-bit
#define CHANNELS 2
// Chunk Duration: 25ms

#define ChunkBytes 4408
#define FRAMES_PER_BUFFER (ChunkBytes / (CHANNELS * sizeof(uint16_t))) // 1102 frames

#define MAX_BUFFER_SIZE 50
#define MAX_OUT_OF_ORDER 10

#define PCM_DATA_SIZE_IN_BYTES ChunkBytes 
#define PCM_DATA_SIZE_IN_ELEMENTS (PCM_DATA_SIZE_IN_BYTES / sizeof(uint16_t))

typedef struct {
    uint32_t PacketNumber;
    uint32_t timestamp_ms;
    uint16_t AudioDataPCM[PCM_DATA_SIZE_IN_ELEMENTS]; 
} AudioPacket;

typedef struct {
    AudioPacket* packets[MAX_BUFFER_SIZE];
    int count;                   
    uint32_t next_expected_seq; 
} AudioBuffer;

uint32_t get_timestamp_ms(void);
AudioPacket* create_audio_packet(uint32_t packet_number, const uint16_t* pcm_data,size_t bytes_read);
void init_circular_buffer(AudioBuffer* buffer);
AudioPacket* GetNextPacket(AudioBuffer* buffer);
int ReceiveBufferPacket(int sock_fd,AudioBuffer* buffer);

#endif