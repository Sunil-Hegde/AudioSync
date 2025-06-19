#include "audio.h"

uint64_t get_timestamp_ms(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint32_t)(tv.tv_sec * 1000 + tv.tv_usec / 1000);
}

// Function to create Audio Packets while sending
AudioPacket *create_audio_packet(uint32_t packet_number, const uint16_t *pcm_data, size_t elements_read)
{
    AudioPacket *packet = malloc(sizeof(AudioPacket));
    if (!packet)
        return NULL;

    packet->PacketNumber = packet_number;
    packet->timestamp_ms = get_timestamp_ms();

    memset(packet->AudioDataPCM, 0, sizeof(packet->AudioDataPCM));

    size_t elements_to_copy = (elements_read < PCM_DATA_SIZE_IN_ELEMENTS) ? elements_read : PCM_DATA_SIZE_IN_ELEMENTS;
    memcpy(packet->AudioDataPCM, pcm_data, elements_to_copy * sizeof(uint16_t));
    return packet;
}

void init_circular_buffer(AudioBuffer *buffer)
{
    memset(buffer->packets, 0, sizeof(buffer->packets));
    buffer->count = 0;
    buffer->next_expected_seq = 0;
}

AudioPacket *GetNextPacket(AudioBuffer *buffer)
{
    int expected_index = buffer->next_expected_seq % MAX_BUFFER_SIZE;

    if (buffer->packets[expected_index] != NULL &&
        buffer->packets[expected_index]->PacketNumber == buffer->next_expected_seq)
    {
        AudioPacket *packet = buffer->packets[expected_index];
        buffer->packets[expected_index] = NULL;
        buffer->next_expected_seq++;
        buffer->count--;
        return packet;
    }
    else
    {
        if (buffer->count > MAX_BUFFER_SIZE / 2)
        {
            for (uint32_t i = 0; i < MAX_BUFFER_SIZE; ++i)
            {
                uint32_t check_seq = buffer->next_expected_seq + i;
                int check_idx = check_seq % MAX_BUFFER_SIZE;
                if (buffer->packets[check_idx] != NULL && buffer->packets[check_idx]->PacketNumber == check_seq)
                {
                    if (i > 0)
                    {
                        printf("Skipping %d missing packets, jumping from %u to %u\n", i, buffer->next_expected_seq, check_seq);
                        buffer->next_expected_seq = check_seq;
                    }
                    expected_index = buffer->next_expected_seq % MAX_BUFFER_SIZE;
                    if (buffer->packets[expected_index] != NULL && buffer->packets[expected_index]->PacketNumber == buffer->next_expected_seq)
                    {
                        AudioPacket *packet = buffer->packets[expected_index];
                        buffer->packets[expected_index] = NULL;
                        buffer->next_expected_seq++;
                        buffer->count--;
                        return packet;
                    }
                    break;
                }
            }
        }
        return NULL;
    }
}

int ReceiveBufferPacket(int sock_fd, AudioBuffer *buffer)
{
    AudioPacket *received_packet = malloc(sizeof(AudioPacket));
    if (!received_packet)
    {
        perror("Failed to allocate memory for packet");
        return -1;
    }

    ssize_t bytes_received = recv(sock_fd, received_packet, sizeof(AudioPacket), 0);

    if (bytes_received < 0)
    {
        perror("Failed to receive packet");
        free(received_packet);
        return -1;
    }

    if (bytes_received == 0)
    {
        printf("Connection closed by sender\n");
        free(received_packet);
        return 0;
    }

    if (bytes_received != sizeof(AudioPacket))
    {
        printf("Warning: Received incomplete packet (%zd bytes)\n", bytes_received);
        free(received_packet);
        return -1;
    }

    printf("Received packet %u, timestamp: %llu ms\n",
           received_packet->PacketNumber, received_packet->timestamp_ms);

    uint32_t buffer_index = received_packet->PacketNumber % MAX_BUFFER_SIZE;
    if (buffer->packets[buffer_index] != NULL)
    {
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
