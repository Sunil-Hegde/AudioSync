#include "audio.h"

// Function to create Audio Packets while sending
AudioPacket *create_audio_packet(uint32_t packet_number, const uint16_t *pcm_data, size_t elements_read){
    AudioPacket *packet = malloc(sizeof(AudioPacket));
    if (!packet)
        return NULL;

    packet->PacketNumber = packet_number;
    packet->timestamp_ns = 0;

    memset(packet->AudioDataPCM, 0, sizeof(packet->AudioDataPCM));

    size_t elements_to_copy = (elements_read < PCM_DATA_SIZE_IN_ELEMENTS) ? elements_read : PCM_DATA_SIZE_IN_ELEMENTS;
    memcpy(packet->AudioDataPCM, pcm_data, elements_to_copy * sizeof(uint16_t));
    return packet;
}

void init_circular_buffer(AudioBuffer *buffer){
    memset(buffer->packets, 0, sizeof(buffer->packets));
    buffer->count = 0;
    buffer->next_expected_seq = 0;
}

// Function to handle mid-stream joining
void reset_buffer_for_midstream(AudioBuffer *buffer) {
    uint32_t min_packet_num = UINT32_MAX;
    int found_packet = 0;
    
    for (int i = 0; i < MAX_BUFFER_SIZE; i++) {
        if (buffer->packets[i] != NULL) {
            if (buffer->packets[i]->PacketNumber < min_packet_num) {
                min_packet_num = buffer->packets[i]->PacketNumber;
                found_packet = 1;
            }
        }
    }
    
    if (found_packet) {
        printf("Mid-stream join detected. Resetting sequence to %u\n", min_packet_num);
        buffer->next_expected_seq = min_packet_num;
    }
}

AudioPacket *GetNextPacket(AudioBuffer *buffer){
    uint64_t current_time = get_timestamp_ns();
    int expected_index = buffer->next_expected_seq % MAX_BUFFER_SIZE;

    // We might be joining mid stream
    if (buffer->count > 3 && buffer->packets[expected_index] == NULL) {
        for (int i = 0; i < MAX_BUFFER_SIZE; i++) {
            if (buffer->packets[i] != NULL) {
                reset_buffer_for_midstream(buffer);
                break;
            }
        }
    }

    if (buffer->packets[expected_index] != NULL &&
        buffer->packets[expected_index]->PacketNumber == buffer->next_expected_seq){
        if (current_time >= buffer->packets[expected_index]->timestamp_ns) {
            AudioPacket *packet = buffer->packets[expected_index];
            buffer->packets[expected_index] = NULL;
            buffer->next_expected_seq++;
            buffer->count--;
            return packet;
        } else {
            return NULL;
        }
    } else {
        if (buffer->count > 2) {
            for (uint32_t i = 0; i < MAX_BUFFER_SIZE; ++i){
                uint32_t check_seq = buffer->next_expected_seq + i;
                int check_idx = check_seq % MAX_BUFFER_SIZE;
                
                if (buffer->packets[check_idx] != NULL && 
                    buffer->packets[check_idx]->PacketNumber == check_seq){
                    if (current_time >= buffer->packets[check_idx]->timestamp_ns) {
                        if (i > 0){
                            printf("Skipping %d missing packets, jumping from %u to %u\n", 
                                   i, buffer->next_expected_seq, check_seq);
                            buffer->next_expected_seq = check_seq;
                        }
                        
                        AudioPacket *packet = buffer->packets[check_idx];
                        buffer->packets[check_idx] = NULL;
                        buffer->next_expected_seq++;
                        buffer->count--;
                        return packet;
                    } else {
                        return NULL;
                    }
                }
            }
        }
        return NULL;
    }
}
