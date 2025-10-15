#include "common.h"

static struct sockaddr_in multicast_addr;
static int64_t current_time_offset = 0;
static int first_sync_received = 0;
static uint64_t stream_start_time = 0;
static uint64_t local_stream_start = 0;

OpusContext *context = NULL;

void SetupSender(int *sock_fd) {
    *sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
    int ttl = MULTICAST_TTL;  
    setsockopt(*sock_fd, IPPROTO_IP, IP_MULTICAST_TTL, &ttl, sizeof(ttl));

    memset(&multicast_addr, 0, sizeof(multicast_addr));
    multicast_addr.sin_family = AF_INET;
    multicast_addr.sin_addr.s_addr = inet_addr(MULTICAST_GROUP);
    multicast_addr.sin_port = htons(MULTICAST_PORT);

    printf("Multicast sender ready for group %s:%d\n", MULTICAST_GROUP, MULTICAST_PORT);
}

void SendData(int *sock_fd, const AudioPacket *packet, size_t packet_size, SyncPacket *sync_packet){
    static uint32_t seq = 0;
    if (seq % 10 == 0){
        get_server_time(sync_packet, seq);
        sendto(*sock_fd, sync_packet, sizeof(SyncPacket), 0,
            (struct sockaddr*)&multicast_addr, sizeof(multicast_addr));
    }
    ssize_t bytes_sent = sendto(*sock_fd, packet, packet_size, 0,
                               (struct sockaddr*)&multicast_addr, sizeof(multicast_addr));
    if (bytes_sent == -1) {
        perror("sender: sendto");
    } else if ((size_t)bytes_sent != packet_size) {
        fprintf(stderr, "sender: partial packet sent (%zd of %zu bytes)\n", 
                bytes_sent, packet_size);
    }
    seq++;
}

void SetupReceiver(int *sock_fd) {
    *sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (*sock_fd < 0) {
        perror("receiver: socket");
        exit(1);
    }
    int reuse = 1;
    if (setsockopt(*sock_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
        perror("receiver: setsockopt SO_REUSEADDR");
        close(*sock_fd);
        exit(1);
    }

    // Bind to multicast port
    struct sockaddr_in local_addr;
    memset(&local_addr, 0, sizeof(local_addr));
    local_addr.sin_family = AF_INET;
    local_addr.sin_addr.s_addr = INADDR_ANY;
    local_addr.sin_port = htons(MULTICAST_PORT);

    if (bind(*sock_fd, (struct sockaddr*)&local_addr, sizeof(local_addr)) < 0) {
        perror("receiver: bind");
        close(*sock_fd);
        exit(1);
    }

    // Join multicast group
    struct ip_mreq mreq;
    mreq.imr_multiaddr.s_addr = inet_addr(MULTICAST_GROUP);
    mreq.imr_interface.s_addr = INADDR_ANY;

    if (setsockopt(*sock_fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0) {
        perror("receiver: join group");
        close(*sock_fd);
        exit(1);
    }

    printf("Receiver joined multicast group %s:%d\n", MULTICAST_GROUP, MULTICAST_PORT);
}

void PacketSetupAndSend(FILE *audio_file) {
    int sock_fd;
    SetupSender(&sock_fd);

    context = opus_initialize();
    
    uint32_t packet_number = 0;
    uint16_t pcm_read_buffer[PCM_DATA_SIZE_IN_ELEMENTS];
    int stream_active = 1;
    
    // Initialize stream start time
    stream_start_time = get_timestamp_ns();

    printf("Starting multicast audio stream at time: %llu\n", stream_start_time);
    printf("Press Ctrl+C to stop\n");

    while(stream_active) {
        uint64_t target_time = stream_start_time + (packet_number * PACKET_INTERVAL_NS);
        uint64_t current_time = get_timestamp_ns();
        if (current_time < target_time) {
            uint64_t wait_ns = target_time - current_time;
            usleep(wait_ns / 1000);
        }
        
        size_t elements_read = fread(pcm_read_buffer, 
                                    sizeof(uint16_t), 
                                   PCM_DATA_SIZE_IN_ELEMENTS, 
                                   audio_file);
        if(elements_read == 0) {
            if (feof(audio_file)) {
                printf("End of file, looping...\n");
                rewind(audio_file);
                continue;
            } else if (ferror(audio_file)) {
                perror("sender: fread");
            }
            stream_active = 0;
            break;
        }
        
        AudioPacket* packet = create_audio_packet(packet_number, pcm_read_buffer, context);
        if (!packet) {
            fprintf(stderr, "Failed to create packet %u\n", packet_number);
            continue;
        }
        
        // Set relative timestamp
        packet->timestamp_ns = packet_number * PACKET_INTERVAL_NS;
    
        SyncPacket sync_packet;
        printf("%lu", sizeof(packet));
        SendData(&sock_fd, packet, sizeof(AudioPacket), &sync_packet);
        
        free(packet);
        packet_number++;
    }

    printf("Streaming complete. Sent %u packets.\n", packet_number);
    close(sock_fd);
}

int ReceiveBufferPacket(int sock_fd, AudioBuffer *buffer) {
    char packet_buffer[sizeof(AudioPacket)];
    ssize_t bytes_received = recv(sock_fd, packet_buffer, sizeof(packet_buffer), 0);
    
    if (bytes_received < 0){
        perror("Failed to receive packet");
        return -1;
    }
    if (bytes_received == 0){
        printf("Connection closed by sender\n");
        return 0;
    }
    
    if (bytes_received == sizeof(SyncPacket)) {
        SyncPacket *sync_packet = (SyncPacket*)packet_buffer;
        get_client_time(sync_packet);
        current_time_offset = sync_packet->client_offset;
        printf("Updated time offset: %lld ns\n", current_time_offset);
        
        if (!first_sync_received) {
            printf("First sync packet received - initializing local stream reference\n");
            // Initialize local stream start time when we get first sync
            local_stream_start = get_timestamp_ns();
            first_sync_received = 1;
        }
        
        return 1;
    } else if (bytes_received == sizeof(AudioPacket)) {
        AudioPacket *received_packet = malloc(sizeof(AudioPacket));
        if (!received_packet) {
            perror("Failed to allocate memory for packet");
            return -1;
        }
        
        memcpy(received_packet, packet_buffer, sizeof(AudioPacket));
        uint64_t relative_timestamp = received_packet->timestamp_ns;
        
        // Calculate when this packet should be played locally
        uint64_t target_play_time = local_stream_start + relative_timestamp;
        
        // If packet time is too far in the future, then adjust
        uint64_t current_time = get_timestamp_ns();
        int64_t time_diff = (int64_t)(target_play_time - current_time);

        if (time_diff > 1000000000) { 
            printf("Adjusting for mid-stream join: packet was %lld ms in future\n", 
                   time_diff / 1000000);
            local_stream_start = current_time - relative_timestamp + 100000000; // 100ms delay
            target_play_time = local_stream_start + relative_timestamp;
            printf("Adjusted local_stream_start to: %llu\n", local_stream_start);
        }

        received_packet->timestamp_ns = target_play_time;
        
        printf("Received packet %u, relative_time: %llu ns, target_play: %llu ns (in %lld ms)\n",
               received_packet->PacketNumber, relative_timestamp, target_play_time, time_diff / 1000000);

        uint32_t buffer_index = received_packet->PacketNumber % MAX_BUFFER_SIZE;
        if (buffer->packets[buffer_index] != NULL){
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
    else {
        printf("Warning: Received packet with unexpected size (%zd bytes)\n", bytes_received);
        return -1;
    }
}

static int networkAudioCallback(
    const void *inputBuffer, void *outputBuffer,
    unsigned long framesPerBuffer,
    const PaStreamCallbackTimeInfo* timeInfo,
    PaStreamCallbackFlags statusFlags,
    void *userData
) {
    (void)inputBuffer; (void)timeInfo; (void)statusFlags;
    
    AudioBuffer *buffer = (AudioBuffer*)userData;
    uint16_t *out = (uint16_t*)outputBuffer;
    size_t samplesToWrite = framesPerBuffer * CHANNELS;
    
    AudioPacket* packet = GetNextPacket(buffer);
    if (packet != NULL) {
        // Decode the Opus compressed data to PCM
        int16_t decoded_pcm[PCM_DATA_SIZE_IN_ELEMENTS];
        int decoded_frames = opus_decode_audio(context, 
                                             packet->OpusEncodedData,
                                             packet->opus_packet_size,
                                             decoded_pcm);
        
        if (decoded_frames > 0) {
            // Copy the decoded PCM data to output
            size_t bytes_to_copy = (samplesToWrite < (size_t)(decoded_frames * CHANNELS)) ? 
                                  samplesToWrite * sizeof(uint16_t) : 
                                  decoded_frames * CHANNELS * sizeof(uint16_t);
            memcpy(out, decoded_pcm, bytes_to_copy);
            
            // Fill remaining with silence if needed
            if (bytes_to_copy < samplesToWrite * sizeof(uint16_t)) {
                memset((uint8_t*)out + bytes_to_copy, 0, 
                       samplesToWrite * sizeof(uint16_t) - bytes_to_copy);
            }
        } else {
            // Decoding failed, output silence
            memset(out, 0, samplesToWrite * sizeof(uint16_t));
            printf("Warning: Opus decode failed for packet %u\n", packet->PacketNumber);
        }
        
        free(packet);
        return paContinue;
    } else {
        // No packet available, output silence
        memset(out, 0, samplesToWrite * sizeof(uint16_t));
        return paContinue; 
    }
}

void ReceiveAudio(AudioBuffer *buffer){
    int sock_fd;
    Pa_Initialize();
    context = opus_initialize();
    
    PaStream *stream;
    PaStreamParameters output;
    output.device = Pa_GetDefaultOutputDevice();
    output.channelCount = CHANNELS;
    output.sampleFormat = paInt16;
    output.suggestedLatency = Pa_GetDeviceInfo(output.device)->defaultLowOutputLatency;
    output.hostApiSpecificStreamInfo = NULL;

    Pa_OpenStream(&stream, NULL, &output,
        SAMPLE_RATE, FRAMES_PER_BUFFER, paClipOff,
        networkAudioCallback, buffer);

    SetupReceiver(&sock_fd);
    memset(buffer->packets, 0, sizeof(buffer->packets));
    buffer->next_expected_seq = 0;

    printf("Buffering initial packets...\n");
    while(buffer->count < 5) {
        int res = ReceiveBufferPacket(sock_fd, buffer);
        if(res > 0) {
            printf("Initial buffering: %d packets\n", buffer->count);
        }
    }
    Pa_StartStream(stream);
    printf("Audio playback started with %d packets buffered\n", buffer->count);
    
    while(1) {
        int res = ReceiveBufferPacket(sock_fd, buffer);
        if(res > 0) {
            printf("Buffered packet, buffer has %d packets\n", buffer->count);
        }
    }
    Pa_StopStream(stream);
    Pa_CloseStream(stream);
    Pa_Terminate();
}