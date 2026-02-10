#ifndef AUDIO_H
#define AUDIO_H

#include "common.h"
#include "codec.h"

typedef struct {
    uint32_t PacketNumber;
    uint64_t timestamp_ns;
    uint8_t OpusEncodedData[MAX_OPUS_PACKET_SIZE];
    int opus_packet_size; 
} AudioPacket;

typedef struct {
    AudioPacket* packets[MAX_BUFFER_SIZE];
    int count;                   
    uint32_t next_expected_seq; 
} AudioBuffer;

typedef struct {
    AudioBuffer *buffer;
    int channel_index;
} CallbackData;

uint64_t get_timestamp_ns(void);
AudioPacket* create_audio_packet(
    uint32_t packet_number,
    const uint16_t* pcm_data,
    OpusContext *context);
void init_circular_buffer(AudioBuffer* buffer);
void reset_buffer_for_midstream(AudioBuffer *buffer);
AudioPacket* GetNextPacket(AudioBuffer* buffer);

#endif