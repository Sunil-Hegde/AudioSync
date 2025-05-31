#ifndef AUDIO_H
#define AUDIO_H

#include <stdio.h>
#include <stdint.h>
#include <sys/time.h>
// Sample Rate: 44.1k
// Sample Format: 16-bit
// Channels: 2
// Chunk Duration: 25ms
#define ChunkBytes 4408

#define MAX_BUFFER_SIZE 1000
#define MAX_OUT_OF_ORDER 10

typedef struct AudioPacket{
    uint32_t PacketNumber;
    uint16_t AudioDataPCM[ChunkBytes];
    uint64_t timestamp_usec;
} AudioPacket;

typedef struct {
    AudioPacket* packets[MAX_BUFFER_SIZE];
    uint32_t next_expected_seq;
    int buffer_count;
} AudioBuffer;

AudioPacket* create_audio_packet(uint32_t packet_number, const uint16_t* pcm_data, size_t data_length);

// Function to get current timestamp in microseconds
uint64_t get_timestamp_usec(void);

void init_audio_buffer(AudioBuffer* buffer);

void cleanup_audio_buffer(AudioBuffer* buffer);

#endif