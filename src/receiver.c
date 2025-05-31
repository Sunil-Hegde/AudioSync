#include "network.h"
#include "audio.h"
#include <string.h>
#include <unistd.h>
#include <errno.h>

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s <sender_ip> <output_file.raw>\n", argv[0]);
        return 1;
    }
    
    const char *server_ip = argv[1];
    const char *output_filename = argv[2];
    
    // Open output file for writing
    FILE *output_file = fopen(output_filename, "wb");
    if (!output_file) {
        perror("Failed to open output file");
        return 1;
    }
    
    int sock_fd;
    AudioBuffer buffer;
    char recv_buffer[1024];
    
    // Setup connection with the sender
    SetupReceiver(server_ip, &sock_fd);
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
            
            while (1) {
                if (receive_and_buffer_packet(sock_fd, &buffer) > 0) {
                    AudioPacket* packet;
                    while ((packet = get_next_ordered_packet(&buffer)) != NULL) {
                        printf("Writing packet %u with timestamp %llu\n", 
                              packet->PacketNumber, (unsigned long long)packet->timestamp_usec);
                        
                        // Write the audio data to the output file
                        size_t samples_written = fwrite(packet->AudioDataPCM, 
                                                       sizeof(uint16_t), 
                                                       ChunkBytes, 
                                                       output_file);
                        
                        if (samples_written != ChunkBytes) {
                            fprintf(stderr, "Warning: Failed to write all samples. Expected %d, wrote %zu\n",
                                   ChunkBytes, samples_written);
                        }
                        
                        // Flush the file to ensure data is written
                        fflush(output_file);
                        
                        total_packets++;
                        total_samples += samples_written;
                        printf("Progress: %u packets, %.2f MB written\n", 
                               total_packets, 
                               (float)(total_samples * sizeof(uint16_t)) / (1024.0 * 1024.0));
                        
                        free(packet);
                    }
                }
                
                // Check for user interrupt (Ctrl+C)
                if (ferror(output_file)) {
                    fprintf(stderr, "Error writing to file: %s\n", strerror(errno));
                    break;
                }
            }
        } else {
            printf("Unexpected response from sender: %s\n", recv_buffer);
        }
    } else {
        perror("Failed to receive confirmation");
    }
    
    // Cleanup
    printf("Closing output file...\n");
    fclose(output_file);
    cleanup_audio_buffer(&buffer);
    close(sock_fd);
    
    printf("Audio reception complete. File saved: %s\n", output_filename);
    printf("Total audio data: %.2f MB\n", 
           (float)(ftell(output_file)) / (1024.0 * 1024.0));
    
    return 0;
}