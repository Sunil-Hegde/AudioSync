#include "network.h"
#include "audio.h"
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>  // Add this include for fcntl functions

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
                int result = receive_and_buffer_packet(sock_fd, &buffer);
                
                if (result > 0) {
                    // Process audio packets
                    AudioPacket* packet;
                    while ((packet = get_next_ordered_packet(&buffer)) != NULL) {
                        printf("Writing packet %u with timestamp %llu\n", 
                              packet->PacketNumber, (unsigned long long)packet->timestamp_usec);
                        
                        // Determine how many samples to write
                        size_t samples_to_write = (packet->data_size > 0) ? packet->data_size : ChunkBytes;
                        
                        // Write the audio data to the output file
                        size_t samples_written = fwrite(packet->AudioDataPCM, 
                                                       sizeof(uint16_t), 
                                                       samples_to_write, 
                                                       output_file);
                        
                        if (samples_written != samples_to_write) {
                            fprintf(stderr, "Warning: Failed to write all samples. Expected %zu, wrote %zu\n",
                                   samples_to_write, samples_written);
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
                } else if (result == 0) {
                    // Check if we received a termination message instead of an audio packet
                    char end_buffer[32];
                    
                    // Set socket to non-blocking mode temporarily
                    int flags = fcntl(sock_fd, F_GETFL, 0);
                    fcntl(sock_fd, F_SETFL, flags | O_NONBLOCK);
                    
                    int end_bytes = recv(sock_fd, end_buffer, sizeof(end_buffer) - 1, 0);
                    
                    // Set socket back to blocking mode
                    fcntl(sock_fd, F_SETFL, flags);
                    
                    if (end_bytes > 0) {
                        end_buffer[end_bytes] = '\0';
                        if (strcmp(end_buffer, "STREAM_COMPLETE") == 0) {
                            printf("Received stream completion message from sender.\n");
                            break;  // Exit the reception loop
                        }
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