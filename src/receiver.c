#include "network.h"
#include "rtp.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define RTP_MAX_PACKET_SIZE 1400

int main() {
    char ServerIP[INET6_ADDRSTRLEN];
    printf("Enter server IP address: ");
    scanf("%s", ServerIP);
    
    int sock_fd;
    uint8_t packet[RTP_MAX_PACKET_SIZE];
    RTPHeader header;

    // Set up connection to server
    SetupReceiver(ServerIP, &sock_fd);
    
    // Open file to save received audio
    FILE *output_file = fopen("received_audio.raw", "wb");
    if (!output_file) {
        perror("Failed to create output file");
        return 1;
    }
    
    printf("Receiving RTP stream. Press Ctrl+C to stop.\n");
    
    uint16_t last_seq = 0;
    int first_packet = 1;
    
    // Receive loop
    while (1) {
        // Send request for data (our protocol expects clients to request first)
        char *request = "READY";
        send(sock_fd, request, strlen(request), 0);
        
        // Receive RTP packet
        ssize_t bytes_received = ReceiveBinaryData(&sock_fd, packet, RTP_MAX_PACKET_SIZE);
        
        if (bytes_received < RTP_HEADER_SIZE) {
            printf("Received packet too small to be RTP\n");
            continue;
        }
        
        // Parse RTP header
        unpack_rtp_header(packet, &header);
        
        // Check for packet loss
        if (!first_packet && header.sequence_number != last_seq + 1) {
            printf("Packet loss detected! Expected seq %u, got %u\n", 
                   last_seq + 1, header.sequence_number);
        }
        
        first_packet = 0;
        last_seq = header.sequence_number;
        
        // Extract and save payload
        size_t payload_size = bytes_received - RTP_HEADER_SIZE;
        fwrite(packet + RTP_HEADER_SIZE, 1, payload_size, output_file);
        fflush(output_file);
        
        printf("Received RTP packet: seq=%u, ts=%u, size=%zu\n", 
               header.sequence_number, header.timestamp, payload_size);
    }
    
    fclose(output_file);
    return 0;
}