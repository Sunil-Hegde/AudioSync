#include "network.h"
#include "rtp.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>

int main() {
    char ServerIP[INET6_ADDRSTRLEN];
    printf("Enter server IP address: ");
    scanf("%s", ServerIP);
    
    int sock_fd;
    uint8_t buffer[RTP_MAX_PACKET_SIZE];
    RTPPacket received_packet;

    // Set up connection to server
    SetupReceiver(ServerIP, &sock_fd);
    
    // Open file to save received audio
    FILE *output_file = fopen("received_audio.raw", "wb");
    if (!output_file) {
        perror("Failed to create output file");
        return 1;
    }
    
    printf("Receiving RTP stream. Press Ctrl+C to stop.\n");
    printf("Saving audio to: received_audio.raw\n");
    
    uint16_t last_seq = 0;
    uint32_t expected_ssrc = 0;
    int first_packet = 1;
    int packets_received = 0;
    int packets_lost = 0;
    
    // Receive loop
    while (1) {
        // Send request for data (handshake with sender)
        char *request = "READY";
        send(sock_fd, request, strlen(request), 0);
        
        // Receive RTP packet
        ssize_t bytes_received = ReceiveBinaryData(&sock_fd, buffer, RTP_MAX_PACKET_SIZE);
        
        if (bytes_received < RTP_HEADER_SIZE) {
            printf("Received packet too small to be RTP (%zd bytes)\n", bytes_received);
            continue;
        }
        
        // Deserialize the received packet
        deserialize_rtp_packet(buffer, bytes_received, &received_packet);
        
        // Validate RTP version
        if (received_packet.header.version != RTP_VERSION) {
            printf("Invalid RTP version: %d\n", received_packet.header.version);
            continue;
        }
        
        // Initialize SSRC on first packet
        if (first_packet) {
            expected_ssrc = received_packet.header.ssrc;
            printf("Initialized SSRC: %u\n", expected_ssrc);
            first_packet = 0;
        } else {
            // Check SSRC consistency
            if (received_packet.header.ssrc != expected_ssrc) {
                printf("SSRC mismatch! Expected: %u, Got: %u\n", 
                       expected_ssrc, received_packet.header.ssrc);
                continue;
            }
            
            // Check for packet loss
            uint16_t expected_seq = last_seq + 1;
            if (received_packet.header.sequence_number != expected_seq) {
                if (received_packet.header.sequence_number > expected_seq) {
                    packets_lost += (received_packet.header.sequence_number - expected_seq);
                    printf("Packet loss detected! Expected seq %u, got %u (lost %d packets)\n", 
                           expected_seq, received_packet.header.sequence_number, 
                           received_packet.header.sequence_number - expected_seq);
                } else {
                    printf("Out-of-order packet! Expected seq %u, got %u\n", 
                           expected_seq, received_packet.header.sequence_number);
                }
            }
        }
        
        last_seq = received_packet.header.sequence_number;
        packets_received++;
        
        // Debug: Print first few bytes of received audio data
        printf("Received audio data (first 8 bytes): ");
        for (size_t i = 0; i < 8 && i < received_packet.payload_length; i++) {
            printf("%02X ", received_packet.payload[i]);
        }
        printf("\n");
        
        // Write payload to output file
        fwrite(received_packet.payload, 1, received_packet.payload_length, output_file);
        fflush(output_file);  // Ensure data is written immediately
        
        printf("Received RTP packet: seq=%u, ts=%u, payload=%zu bytes, total_received=%d\n", 
               received_packet.header.sequence_number, 
               received_packet.header.timestamp,
               received_packet.payload_length,
               packets_received);
        
        // Print statistics every 100 packets
        if (packets_received % 100 == 0) {
            float loss_rate = (float)packets_lost / (packets_received + packets_lost) * 100.0;
            printf("Statistics: Received=%d, Lost=%d, Loss Rate=%.2f%%\n", 
                   packets_received, packets_lost, loss_rate);
        }
    }
    
    fclose(output_file);
    return 0;
}