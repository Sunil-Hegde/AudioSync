#include "network.h"
#include "audio.h"
#include <unistd.h>
#include <stdint.h>
#include <sys/socket.h>
#include <stdio.h>      // For printf, perror
#include <string.h>     // For memset, strlen
#include <netdb.h>      // For getaddrinfo, addrinfo
#include <arpa/inet.h>  // For inet_ntop, inet_pton
#include <stdlib.h>     // For malloc, free



void SetupSender(int *sock_fd){
    struct addrinfo server_description, *server;
    char server_ip[INET6_ADDRSTRLEN];

    memset(&server_description, 0, sizeof server_description);
    server_description.ai_family = AF_INET;
    server_description.ai_socktype = SOCK_DGRAM;

    getaddrinfo("0.0.0.0", PORT, &server_description, &server);
    *sock_fd = socket(server->ai_family, server->ai_socktype, server->ai_protocol);
    inet_ntop(AF_INET, &((struct sockaddr_in *)server->ai_addr)->sin_addr, server_ip, sizeof server_ip);
    bind(*sock_fd, server->ai_addr, server->ai_addrlen);

    printf("UDP Server running at IP: %s and port %s.\n", server_ip, PORT);
}

void send_audio_packet(int sock_fd, const AudioPacket* packet) {
    struct sockaddr_in client_addr;
    
    // Set up client address (receiver)
    memset(&client_addr, 0, sizeof(client_addr));
    client_addr.sin_family = AF_INET;
    client_addr.sin_port = htons(12346); // Client listening port
    inet_pton(AF_INET, "127.0.0.1", &client_addr.sin_addr); // Localhost or target IP
    
    // Send the audio packet directly
    ssize_t bytes_sent = sendto(sock_fd, packet, sizeof(AudioPacket), 0,
                               (struct sockaddr*)&client_addr, sizeof(client_addr));
    
    if (bytes_sent < 0) {
        perror("Failed to send audio packet");
    }
}

void SendData(int *sock_fd, char *message){
    struct sockaddr_storage client_addr;
    socklen_t client_addr_size = sizeof client_addr;
    char client_ip[INET6_ADDRSTRLEN];
    char buffer[1024];

    int bytes_received = recvfrom(*sock_fd, buffer, sizeof(buffer) - 1, 0, (struct sockaddr *)&client_addr, &client_addr_size);

    if (bytes_received > 0) {
        buffer[bytes_received] = '\0';
        
        struct sockaddr_in *client_in = (struct sockaddr_in*)&client_addr;
        inet_ntop(AF_INET, &client_in->sin_addr, client_ip, sizeof(client_ip));
        printf("Received from client %s: %s\n", client_ip, buffer);
        
        while(1){
            sendto(*sock_fd, message, strlen(message), 0, (struct sockaddr*)&client_addr, client_addr_size);
            printf("Sent response to client %s\n", client_ip);
            sleep(1);
        }
    } else {
        perror("recvfrom failed");
    }
}

void SetupReceiver(const char *ServerIP, int *sock_fd){
    struct addrinfo server_description, *server;
    int status;

    memset(&server_description, 0, sizeof server_description);
    server_description.ai_family = AF_INET;
    server_description.ai_socktype = SOCK_DGRAM;

    getaddrinfo(ServerIP, PORT, &server_description, &server);
    *sock_fd = socket(server->ai_family, server->ai_socktype, server->ai_protocol);

    printf("Setting up client to server at IP: %s\n", ServerIP);

    status = connect(*sock_fd, server->ai_addr, server->ai_addrlen);

    if(status != -1){
        printf("Connected to server!\n");
        char *message_from_client = "Hello!";
        send(*sock_fd, message_from_client, strlen(message_from_client), 0);
        printf("Sent initial message to server\n");
    } else {
        printf("Invalid IP\n");
    }
}

void ReceiveData(int *sock_fd, char *buffer){
    while(1){
        int bytes_received = recv(*sock_fd, buffer, 1023, 0);
        if (bytes_received > 0) {
            buffer[bytes_received] = '\0';
            printf("Message from server: %s\n", buffer);
        } else {
            perror("recv");
        }
    }
}

int receive_and_buffer_packet(int sock_fd, AudioBuffer* buffer) {
    AudioPacket* received_packet = malloc(sizeof(AudioPacket));
    if (!received_packet) {
        perror("Failed to allocate memory for packet");
        return -1;
    }
    
    struct sockaddr_in sender_addr;
    socklen_t addr_len = sizeof(sender_addr);
    
    // Receive packet using recvfrom instead of recv for UDP
    ssize_t bytes_received = recvfrom(sock_fd, received_packet, sizeof(AudioPacket), 0,
                                     (struct sockaddr*)&sender_addr, &addr_len);
    
    if (bytes_received < 0) {
        perror("Failed to receive packet");
        free(received_packet);
        return -1;
    }
    
    if (bytes_received != sizeof(AudioPacket)) {
        printf("Warning: Received incomplete packet (%zd bytes)\n", bytes_received);
        free(received_packet);
        return 0;
    }
    
    printf("Received packet %u, timestamp: %llu\n", 
           received_packet->PacketNumber, (unsigned long long)received_packet->timestamp_usec);
    
    // Check if buffer is full
    if (buffer->buffer_count >= MAX_BUFFER_SIZE) {
        printf("Buffer full, dropping oldest packet\n");
        // Find and remove oldest packet
        uint32_t oldest_seq = UINT32_MAX;
        int oldest_idx = -1;
        for (int i = 0; i < MAX_BUFFER_SIZE; i++) {
            if (buffer->packets[i] && buffer->packets[i]->PacketNumber < oldest_seq) {
                oldest_seq = buffer->packets[i]->PacketNumber;
                oldest_idx = i;
            }
        }
        if (oldest_idx >= 0) {
            free(buffer->packets[oldest_idx]);
            buffer->packets[oldest_idx] = NULL;
            buffer->buffer_count--;
        }
    }
    
    // Find empty slot and insert packet
    for (int i = 0; i < MAX_BUFFER_SIZE; i++) {
        if (buffer->packets[i] == NULL) {
            buffer->packets[i] = received_packet;
            buffer->buffer_count++;
            return 1;  // Successfully added packet
        }
    }
    
    // If we reach here, we couldn't find an empty slot despite checking buffer_count
    printf("Error: Buffer inconsistency detected\n");
    free(received_packet);
    return -1;
}

AudioPacket* get_next_ordered_packet(AudioBuffer* buffer) {
    // Look for the next expected sequence number
    for (int i = 0; i < MAX_BUFFER_SIZE; i++) {
        if (buffer->packets[i] && 
            buffer->packets[i]->PacketNumber == buffer->next_expected_seq) {
            
            AudioPacket* packet = buffer->packets[i];
            buffer->packets[i] = NULL;
            buffer->buffer_count--;
            buffer->next_expected_seq++;
            
            printf("Delivering packet %u in order\n", packet->PacketNumber);
            return packet;
        }
    }
    
    // Check if we should skip missing packets (if too far behind)
    uint32_t min_buffered_seq = UINT32_MAX;
    for (int i = 0; i < MAX_BUFFER_SIZE; i++) {
        if (buffer->packets[i] && 
            buffer->packets[i]->PacketNumber < min_buffered_seq) {
            min_buffered_seq = buffer->packets[i]->PacketNumber;
        }
    }
    
    // If we have packets but they're too far ahead, skip missing ones
    if (min_buffered_seq != UINT32_MAX && 
        min_buffered_seq > buffer->next_expected_seq + MAX_OUT_OF_ORDER) {
        
        printf("Skipping missing packets %u to %u\n", 
               buffer->next_expected_seq, min_buffered_seq - 1);
        buffer->next_expected_seq = min_buffered_seq;
        
        // Try again with updated sequence number
        return get_next_ordered_packet(buffer);
    }
    
    return NULL; // No packet ready for delivery
}

void setup_and_stream_audio(FILE *audio_file) {
    int sock_fd;
    
    // Setup the sender socket
    SetupSender(&sock_fd);
    
    // Wait for receiver to connect
    struct sockaddr_storage client_addr;
    socklen_t client_addr_size = sizeof client_addr;
    char client_ip[INET6_ADDRSTRLEN];
    char buffer[1024];

    printf("Waiting for receiver to connect...\n");
    int bytes_received = recvfrom(sock_fd, buffer, sizeof(buffer) - 1, 0, 
                                 (struct sockaddr *)&client_addr, &client_addr_size);

    if (bytes_received <= 0) {
        perror("Failed to receive initial connection");
        return;
    }
    
    buffer[bytes_received] = '\0';
    struct sockaddr_in *client_in = (struct sockaddr_in*)&client_addr;
    inet_ntop(AF_INET, &client_in->sin_addr, client_ip, sizeof(client_ip));
    printf("Receiver connected from %s: %s\n", client_ip, buffer);
    
    // Send confirmation message
    const char *confirm_msg = "READY_TO_STREAM";
    sendto(sock_fd, confirm_msg, strlen(confirm_msg), 0, 
          (struct sockaddr*)&client_addr, client_addr_size);
    
    // Start sending audio packets
    printf("Starting audio stream...\n");
    uint32_t packet_number = 0;
    uint16_t audio_buffer[ChunkBytes];
    
    while(1) {
        // Read audio data from file
        size_t bytes_read = fread(audio_buffer, sizeof(uint16_t), ChunkBytes, audio_file);
        
        if (bytes_read == 0) {
            printf("End of file reached, rewinding...\n");
            rewind(audio_file);
            continue;
        }
        
        // Create audio packet
        AudioPacket* packet = create_audio_packet(packet_number, audio_buffer, bytes_read);
        if (!packet) {
            fprintf(stderr, "Failed to create audio packet\n");
            continue;
        }
        
        // Send the packet - using the client address we got from the initial connection
        ssize_t bytes_sent = sendto(sock_fd, packet, sizeof(AudioPacket), 0,
                               (struct sockaddr*)&client_addr, client_addr_size);
        
        if (bytes_sent < 0) {
            perror("Failed to send audio packet");
        } else {
            printf("Sent packet %u\n", packet->PacketNumber);
        }
        
        // Clean up
        free(packet);
        packet_number++;
        
        // Control the sending rate
        usleep(20000); // 20ms delay between packets, adjust as needed
    }
}
