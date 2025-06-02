#include "audio.h"
#include <stdlib.h>  // for malloc
#include <string.h>  // for memset, memcpy
#include <sys/time.h> // for gettimeofday


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
    // buffer->head = 0;
    // buffer->tail = 0;
    buffer->count = 0;
    buffer->next_expected_seq = 0;
}

AudioPacket* GetNextPacket(AudioBuffer* buffer) {
 
      if (buffer->count == 0) {
        printf("Buffer empty, no packets to deliver\n");
        return NULL;
    }

     uint32_t expected_index = buffer->next_expected_seq % MAX_BUFFER_SIZE;
      if (buffer->packets[expected_index] != NULL && 
        buffer->packets[expected_index]->PacketNumber == buffer->next_expected_seq) {
        
        AudioPacket* packet = buffer->packets[expected_index];
        buffer->packets[expected_index] = NULL;  
        buffer->count--;
        buffer->next_expected_seq++;
        
        printf("Delivering packet %u in order\n", packet->PacketNumber);
        return packet;
    }
    
    printf("Packet %u not ready yet (expected at index %u)\n", 
           buffer->next_expected_seq, expected_index);
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
        return -1;
    }

    printf("Received packet %u, timestamp: %u ms\n", 
       received_packet->PacketNumber, received_packet->timestamp_ms);

        uint32_t buffer_index = received_packet->PacketNumber % MAX_BUFFER_SIZE;
        if (buffer->packets[buffer_index] != NULL) {
        printf("Replacing packet at index %u\n", buffer_index);
        free(buffer->packets[buffer_index]);
        buffer->count--;
        }
         buffer->packets[buffer_index] = received_packet;
        buffer->count++;
         printf("Buffered packet %u at index %u (total: %d packets)\n", 
           received_packet->PacketNumber, buffer_index, buffer->count);
    
    return 1; 
    

}
