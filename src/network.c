#include "network.h"
#include "audio.h"


void SetupSender(int *sock_fd) {
    *sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
    int ttl = MULTICAST_TTL;  
    setsockopt(*sock_fd, IPPROTO_IP, IP_MULTICAST_TTL, &ttl, sizeof(ttl));
    printf("Multicast sender ready for group %s:%d\n", MULTICAST_GROUP, MULTICAST_PORT);
}

void SendData(int *sock_fd, const AudioPacket *packet, size_t packet_size) {
    struct sockaddr_in multicast_addr;
    memset(&multicast_addr, 0, sizeof(multicast_addr));
    multicast_addr.sin_family = AF_INET;
    multicast_addr.sin_addr.s_addr = inet_addr(MULTICAST_GROUP);
    multicast_addr.sin_port = htons(MULTICAST_PORT);

    ssize_t bytes_sent = sendto(*sock_fd, packet, packet_size, 0,
                               (struct sockaddr*)&multicast_addr, sizeof(multicast_addr));
    if (bytes_sent == -1) {
        perror("sender: sendto");
    } else if ((size_t)bytes_sent != packet_size) {
        fprintf(stderr, "sender: partial packet sent (%zd of %zu bytes)\n", 
                bytes_sent, packet_size);
    }
}

void SetupReceiver(const char *ServerIP, int *sock_fd) {
    (void)ServerIP; // Ignore ServerIP parameter for multicast
    
    *sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (*sock_fd < 0) {
        perror("receiver: socket");
        exit(1);
    }

    // Allow multiple receivers on same machine
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
    
    uint32_t packet_number = 0;
    uint16_t pcm_read_buffer[PCM_DATA_SIZE_IN_ELEMENTS];
    int stream_active = 1;

    printf("Starting multicast audio stream\n");
    printf("Press Ctrl+C to stop\n");

    while(stream_active) {
        size_t elements_read = fread(pcm_read_buffer, sizeof(uint16_t), 
                                   PCM_DATA_SIZE_IN_ELEMENTS, audio_file);
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

        AudioPacket* packet = create_audio_packet(packet_number, pcm_read_buffer, elements_read);
        if (!packet) {
            fprintf(stderr, "Failed to create packet %u\n", packet_number);
            continue;
        }
        
        SendData(&sock_fd, packet, sizeof(AudioPacket));
        
        free(packet);
        packet_number++;
        usleep(25000); // ~25ms for proper timing
    }

    printf("Streaming complete. Sent %u packets.\n", packet_number);
    close(sock_fd);
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
    int16_t *out = (int16_t*)outputBuffer;
    size_t samplesToWrite = framesPerBuffer * CHANNELS;

    AudioPacket* packet = GetNextPacket(buffer);
    if (packet != NULL) {
        memcpy(out, packet->AudioDataPCM, samplesToWrite * sizeof(int16_t));
        free(packet);
        return paContinue;
    } else {
        memset(out, 0, samplesToWrite * sizeof(int16_t));
        return paContinue; 
    }
}

void ReceiveAudio(const char *ServerIP, AudioBuffer *buffer){
    int sock_fd;
    FILE *output_file = fopen("output.raw", "wb");
    if (!output_file) {
        perror("Failed to open output file");
        return;
    }
    Pa_Initialize();
    
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

    SetupReceiver(ServerIP, &sock_fd);
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
    fclose(output_file);
}