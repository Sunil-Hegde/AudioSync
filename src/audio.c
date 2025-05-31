#include "audio.h"
#include <stdlib.h>  // for malloc
#include <string.h>  // for memset, memcpy
#include <sys/time.h> // for gettimeofday

uint64_t get_timestamp_usec(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)tv.tv_sec * 1000000 + tv.tv_usec;
}

AudioPacket* create_audio_packet(uint32_t packet_number, const uint16_t* pcm_data, size_t data_length) {
    AudioPacket* packet = malloc(sizeof(AudioPacket));
    if (!packet) {
        return NULL;
    }
    
    packet->PacketNumber = packet_number;
    packet->timestamp_usec = get_timestamp_usec();
    packet->data_size = data_length;  // Add this field to the AudioPacket struct
    
    // Zero out the entire buffer first
    memset(packet->AudioDataPCM, 0, sizeof(packet->AudioDataPCM));
    
    // Only copy the actual data we read
    size_t copy_length = (data_length < ChunkBytes) ? data_length : ChunkBytes;
    memcpy(packet->AudioDataPCM, pcm_data, copy_length * sizeof(uint16_t));
    
    return packet;
}

void init_audio_buffer(AudioBuffer* buffer) {
    memset(buffer->packets, 0, sizeof(buffer->packets));
    buffer->next_expected_seq = 0;
    buffer->buffer_count = 0;
}

void cleanup_audio_buffer(AudioBuffer* buffer) {
    for (int i = 0; i < MAX_BUFFER_SIZE; i++) {
        if (buffer->packets[i]) {
            free(buffer->packets[i]);
            buffer->packets[i] = NULL;
        }
    }
    buffer->buffer_count = 0;
}