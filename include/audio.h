#ifndef AUDIO_H
#define AUDIO_H

#include "common.h"
#include "network.h"
#include "ntp.h"
#include "codec.h"

#define SAMPLE_RATE 48000
// Sample Format: 16-bit
#define CHANNELS 2
// Chunk Duration: 25ms

#define ChunkBytes 3840
#define FRAMES_PER_BUFFER (ChunkBytes / (CHANNELS * sizeof(uint16_t))) // 1200 frames

#define MAX_BUFFER_SIZE 50
#define MAX_OUT_OF_ORDER 10

#define PCM_DATA_SIZE_IN_BYTES ChunkBytes 
#define PCM_DATA_SIZE_IN_ELEMENTS (PCM_DATA_SIZE_IN_BYTES / sizeof(uint16_t))
#define MAX_OPUS_PACKET_SIZE 4000
typedef struct {
    uint32_t PacketNumber;
    uint64_t timestamp_ns;
    uint8_t AudioDataPCM[MAX_OPUS_PACKET_SIZE];
    int opus_packet_size; 
} AudioPacket;

typedef struct {
    AudioPacket* packets[MAX_BUFFER_SIZE];
    int count;                   
    uint32_t next_expected_seq; 
} AudioBuffer;

uint64_t get_timestamp_ns(void);
AudioPacket* create_audio_packet(uint32_t packet_number, const uint16_t* pcm_data,size_t bytes_read, OpusContext *context);
void init_circular_buffer(AudioBuffer* buffer);
void reset_buffer_for_midstream(AudioBuffer *buffer);
AudioPacket* GetNextPacket(AudioBuffer* buffer);

#endif