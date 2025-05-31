#include "network.h"
#include "rtp.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <audio_file.raw>\n", argv[0]);
        return 1;
    }

    int sock_fd;
    SetupSender(&sock_fd);
    
    // Open audio file
    FILE *audio_file = fopen(argv[1], "rb");
    if (!audio_file) {
        perror("Failed to open audio file");
        return 1;
    }
    
    // Initialize RTP parameters
    uint16_t seq_num = 0;
    uint32_t timestamp = 0;
    uint32_t ssrc = rand();  // Random SSRC identifier
    
    RTPPacket rtp_packet;
    uint8_t serialized_buffer[RTP_MAX_PACKET_SIZE];
    uint8_t audio_buffer[RTP_MAX_PAYLOAD_SIZE];
    size_t packet_size;
    
    printf("Starting RTP streaming from file: %s\n", argv[1]);
    printf("SSRC: %u\n", ssrc);
    
    // Streaming loop
    while (1) {
        // Read audio data
        size_t bytes_read = fread(audio_buffer, 1, RTP_MAX_PAYLOAD_SIZE, audio_file);
        
        if (bytes_read == 0) {
            // End of file, rewind to beginning for continuous streaming
            printf("End of file reached, rewinding...\n");
            rewind(audio_file);
            continue;
        }
        
        // Debug: Print first few bytes of audio data
        printf("Audio data (first 8 bytes): ");
        for (size_t i = 0; i < 8 && i < bytes_read; i++) {
            printf("%02X ", audio_buffer[i]);
        }
        printf("\n");
        
        // Create RTP packet
        create_rtp_packet(&rtp_packet, seq_num, timestamp, ssrc, audio_buffer, bytes_read);
        
        // Serialize packet for transmission
        serialize_rtp_packet(&rtp_packet, serialized_buffer, &packet_size);
        
        // Send RTP packet
        SendBinaryData(&sock_fd, serialized_buffer, packet_size);
        printf("Sent RTP packet: seq=%u, ts=%u, size=%zu, payload=%zu bytes\n", 
               seq_num, timestamp, packet_size, bytes_read);
        
        // Update sequence number and timestamp
        seq_num++;
        timestamp += bytes_read / 2;  // For 16-bit PCM, each sample is 2 bytes
        
        // Small delay to control streaming rate (20ms)
        usleep(20000);
    }
    
    fclose(audio_file);
    return 0;
}