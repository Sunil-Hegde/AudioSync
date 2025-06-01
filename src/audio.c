#include "audio.h"

uint64_t get_timestamp_ms(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint32_t)(tv.tv_sec * 1000 + tv.tv_usec / 1000);
}

AudioPacket* create_audio_packet(uint32_t packet_number, const uint16_t* pcm_data,size_t bytes_read) {
    AudioPacket* packet = malloc(sizeof(AudioPacket));
  
    packet->PacketNumber = packet_number;
    packet->timestamp_ms = get_timestamp_ms();

    memset(packet->AudioDataPCM, 0, sizeof(packet->AudioDataPCM));

    size_t copy_length = (bytes_read < ChunkBytes) ? bytes_read : ChunkBytes;
    memcpy(packet->AudioDataPCM, pcm_data, copy_length * sizeof(uint16_t));
    return packet;
}

void init_circular_buffer(AudioBuffer* buffer) {
    memset(buffer->packets, 0, sizeof(buffer->packets));
    buffer->head = 0;
    buffer->tail = 0;
    buffer->count = 0;
    buffer->next_expected_seq = 0;
}

AudioPacket* GetNextPacket(AudioBuffer* buffer) {
 
    for (int i = 0; i < buffer->count; i++) {
        int idx = (buffer->tail + i) % MAX_BUFFER_SIZE;
        if (buffer->packets[idx] &&
            buffer->packets[idx]->PacketNumber == buffer->next_expected_seq) {

            AudioPacket* packet = buffer->packets[idx];

        
            for (int j = i; j < buffer->count - 1; j++) {
                int curr_idx = (buffer->tail + j) % MAX_BUFFER_SIZE;
                int next_idx = (buffer->tail + j + 1) % MAX_BUFFER_SIZE;
                buffer->packets[curr_idx] = buffer->packets[next_idx];
            }

        
            int last_idx = (buffer->tail + buffer->count - 1) % MAX_BUFFER_SIZE;
            buffer->packets[last_idx] = NULL;
            buffer->count--;

            buffer->next_expected_seq++;
            printf("Delivering packet %u in order\n", packet->PacketNumber);
            return packet;
        }
    }


    uint32_t min_seq = UINT32_MAX;
    for (int i = 0; i < buffer->count; i++) {
        int idx = (buffer->tail + i) % MAX_BUFFER_SIZE;
        if (buffer->packets[idx] &&
            buffer->packets[idx]->PacketNumber < min_seq) {
            min_seq = buffer->packets[idx]->PacketNumber;
        }
    }

    if (min_seq != UINT32_MAX &&
        min_seq > buffer->next_expected_seq + MAX_OUT_OF_ORDER) {
        printf("Gap detected: skipping packets %u to %u\n",
               buffer->next_expected_seq, min_seq - 1);
        buffer->next_expected_seq = min_seq;
        return GetNextPacket(buffer); 
    }

    return NULL; 
}


int ReceiveBufferPacket(int sock_fd,AudioBuffer* buffer)
{
    AudioPacket* received_packet = malloc(sizeof(AudioPacket));
    if (!received_packet) {
        perror("Failed to allocate memory for packet");
        return -1;
    }

    ssize_t bytes_received=recv(sock_fd,received_packet,sizeof(AudioPacket),0);

    if (bytes_received < 0) {
        perror("Failed to receive packet");
        free(received_packet);
        return -1;
    }
    
    if (bytes_received == 0) {
        printf("Connection closed by sender\n");
        free(received_packet);
        return 0;
    }
    
    if (bytes_received != sizeof(AudioPacket)) {
        printf("Warning: Received incomplete packet (%zd bytes)\n", bytes_received);
        free(received_packet);
        return 0;
    }

    printf("Received packet %u, timestamp: %u ms\n", 
       received_packet->PacketNumber, received_packet->timestamp_ms);

     if (buffer->count >= MAX_BUFFER_SIZE) {
    
        free(buffer->packets[buffer->tail]);
        buffer->packets[buffer->tail] = NULL;
        buffer->tail = (buffer->tail + 1) % MAX_BUFFER_SIZE;
        buffer->count--;
    }
    
    buffer->packets[buffer->head] = received_packet;
    buffer->head = (buffer->head + 1) % MAX_BUFFER_SIZE;
    buffer->count++;
    return 1;
}

