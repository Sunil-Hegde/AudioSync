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
#include <fcntl.h>      // For fcntl



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

void send_audio_packet(int sock_fd, const AudioPacket* packet, const struct sockaddr* client_addr, socklen_t addr_len) {
  
    ssize_t bytes_sent = sendto(sock_fd, packet, sizeof(AudioPacket), 0,
                               client_addr, addr_len);
    
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
   
    int done = 0;
    
    while(!done) {
        // Read audio data from file
        size_t bytes_read = fread(audio_buffer, sizeof(uint16_t), ChunkBytes, audio_file);
        
        if (bytes_read == 0) {
            printf("End of file reached, stopping stream.\n");
            done = 1;
            break;
        }
        
        // Create audio packet with the actual number of bytes read
        AudioPacket* packet = create_audio_packet(packet_number, audio_buffer, bytes_read);
        if (!packet) {
            fprintf(stderr, "Failed to create audio packet\n");
            continue;
        }
        
        // Log actual data size being sent
        printf("Sent packet %u with %zu samples\n", packet->PacketNumber, bytes_read);
        
        // Send the packet using the client address we got from the initial connection
        ssize_t bytes_sent = sendto(sock_fd, packet, sizeof(AudioPacket), 0,
                              (struct sockaddr*)&client_addr, client_addr_size);
        
        if (bytes_sent < 0) {
            perror("Failed to send audio packet");
        }
        
        // Clean up
        free(packet);
        packet_number++;
        
        // Control the sending rate
        usleep(20000); // 20ms delay between packets, adjust as needed
    }
    
    // Add proper cleanup and shutdown when streaming is complete
    printf("Audio streaming complete. Sent %u packets.\n", packet_number);
    
    // Send a termination message to the receiver
    const char *end_msg = "STREAM_COMPLETE";
    sendto(sock_fd, end_msg, strlen(end_msg), 0, 
           (struct sockaddr*)&client_addr, client_addr_size);
    
    // Close the socket
    close(sock_fd);
    return;
}

// Add new function that uses the existing SetupReceiver
void ReceiveAudio(const char *ServerIP, const char *output_filename) {
    int sock_fd;
    AudioBuffer buffer;
    char recv_buffer[1024];
    
    // Open output file for writing
    FILE *output_file = fopen(output_filename, "wb");
    if (!output_file) {
        perror("Failed to open output file");
        return;
    }
    
    // Use existing SetupReceiver function
    SetupReceiver(ServerIP, &sock_fd);
    init_audio_buffer(&buffer);
    
    printf("Waiting for stream confirmation...\n");
    
    // Wait for confirmation message
    int bytes_received = recv(sock_fd, recv_buffer, sizeof(recv_buffer) - 1, 0);
    if (bytes_received > 0) {
        recv_buffer[bytes_received] = '\0';
        printf("Received from sender: %s\n", recv_buffer);
        
        if (strcmp(recv_buffer, "READY_TO_STREAM") == 0) {
            printf("Starting audio reception...\n");
            printf("Saving audio to: %s\n", output_filename);
            
            uint32_t total_packets = 0;
            uint32_t total_samples = 0;
            
            // Receive audio packets
            while (1) {
                int result = receive_and_buffer_packet(sock_fd, &buffer);
                
                if (result > 0) {
                    // Process audio packets
                    AudioPacket* packet;
                    while ((packet = get_next_ordered_packet(&buffer)) != NULL) {
                        // Write packet data to file
                        size_t samples_to_write = (packet->data_size > 0) ? packet->data_size : ChunkBytes;
                        
                        size_t samples_written = fwrite(packet->AudioDataPCM, 
                                                       sizeof(uint16_t), 
                                                       samples_to_write, 
                                                       output_file);
                        
                        total_packets++;
                        total_samples += samples_written;
                        printf("Progress: %u packets, %.2f MB written\n", 
                               total_packets, 
                               (float)(total_samples * sizeof(uint16_t)) / (1024.0 * 1024.0));
                        
                        free(packet);
                    }
                } else if (result == 0) {
                    // Check for termination message
                    char end_buffer[32];
                    
                    int flags = fcntl(sock_fd, F_GETFL, 0);
                    fcntl(sock_fd, F_SETFL, flags | O_NONBLOCK);
                    
                    int end_bytes = recv(sock_fd, end_buffer, sizeof(end_buffer) - 1, 0);
                    
                    fcntl(sock_fd, F_SETFL, flags);
                    
                    if (end_bytes > 0) {
                        end_buffer[end_bytes] = '\0';
                        if (strcmp(end_buffer, "STREAM_COMPLETE") == 0) {
                            printf("Received stream completion message from sender.\n");
                            break;
                        }
                    }
                }
            }
        }
    }
    
    // Cleanup
    printf("Audio reception complete. File saved: %s\n", output_filename);
    printf("Total audio data: %.2f MB\n", 
           (float)(ftell(output_file)) / (1024.0 * 1024.0));
    
    fclose(output_file);
    cleanup_audio_buffer(&buffer);
    close(sock_fd);
}
