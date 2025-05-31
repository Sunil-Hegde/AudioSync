#include "network.h"
#include "rtp.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>

#define RTP_MAX_PACKET_SIZE 1400   // Keep below typical MTU size
#define RTP_PAYLOAD_SIZE (RTP_MAX_PACKET_SIZE - RTP_HEADER_SIZE)

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
    
    // RTP parameters
    uint16_t seq_num = 0;
    uint32_t timestamp = 0;
    uint32_t ssrc = rand();  // Random SSRC identifier
    
    // Buffer for the complete RTP packet
    uint8_t packet[RTP_MAX_PACKET_SIZE];
    uint8_t *payload = packet + RTP_HEADER_SIZE;
    
    printf("Starting RTP streaming from file: %s\n", argv[1]);
    
    // Streaming loop
    while (1) {
        // Prepare RTP header
        RTPHeader header;
        header.version = RTP_VERSION;
        header.padding = 0;
        header.extension = 0;
        header.csrc_count = 0;
        header.marker = 0;
        header.payload_type = RTP_PAYLOAD_TYPE_PCM;
        header.sequence_number = seq_num++;
        header.timestamp = timestamp;
        header.ssrc = ssrc;
        
        // Pack RTP header into packet
        pack_rtp_header(&header, packet);
        
        // Read audio data into payload
        size_t bytes_read = fread(payload, 1, RTP_PAYLOAD_SIZE, audio_file);
        
        if (bytes_read == 0) {
            // End of file, rewind to beginning for continuous streaming
            rewind(audio_file);
            continue;
        }
        
        // Send RTP packet
        SendBinaryData(&sock_fd, packet, RTP_HEADER_SIZE + bytes_read);
        printf("Sent RTP packet: seq=%u, ts=%u, size=%zu\n", 
               header.sequence_number, header.timestamp, bytes_read + RTP_HEADER_SIZE);
        
        // Update timestamp (assuming 8kHz sample rate, 160 samples per packet)
        timestamp += bytes_read / 2;  // For 16-bit PCM, each sample is 2 bytes
        
        // Add small delay to simulate real-time streaming
        struct timespec ts = {0, 20000000};  // 20ms
        nanosleep(&ts, NULL);
    }
    
    fclose(audio_file);
    return 0;
}