#include "network.h"
#include "audio.h"

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



void SendData(int *sock_fd, const AudioPacket *packet, size_t packet_size){
    static struct sockaddr_storage client_addr;  // Make static
    static socklen_t client_addr_size = sizeof(client_addr);  // Make static
    static int client_connected = 0;  // Make static - persists between calls
    
    if(!client_connected)
    {
        char client_ip[INET6_ADDRSTRLEN];
        char buffer[1024];

        int bytes_received = recvfrom(*sock_fd, buffer, sizeof(buffer) - 1, 0, (struct sockaddr *)&client_addr, &client_addr_size);

        if (bytes_received > 0) {
            buffer[bytes_received] = '\0';
            struct sockaddr_in *client_in = (struct sockaddr_in*)&client_addr;
            inet_ntop(AF_INET, &client_in->sin_addr, client_ip, sizeof(client_ip));
            printf("Received from client %s: %s\n", client_ip, buffer);
            client_connected = 1;  // This will now persist
            const char *confirm_msg = "READY_TO_STREAM";
            sendto(*sock_fd, confirm_msg, strlen(confirm_msg), 0, (struct sockaddr*)&client_addr, client_addr_size);
        } else {
            perror("recvfrom failed");
        }
    }

    if(client_connected){
        ssize_t bytes_sent = sendto(*sock_fd, packet, packet_size, 0, (struct sockaddr*)&client_addr, client_addr_size);
        if (bytes_sent > 0) {
            printf("Sent packet %u\n", packet->PacketNumber);
        } else {
            perror("Failed to send packet");
            client_connected = 0;  
        }
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

int ReceiveData(int *sock_fd, char *buffer){
    while(1){
        int bytes_received = recv(*sock_fd, buffer, 1023, 0);
        if (bytes_received > 0) {
            buffer[bytes_received] = '\0';
            printf("Message from server: %s\n", buffer);
            if(strcmp(buffer,"READY_TO_STREAM")==0)
            {
                printf("Receiving audio..");
                return 1;
                
            }
        } else {
            perror("recv");
            return 0;
        }
    }
}

void PacketSetupAndSend(FILE *audio_file)
{
    int sock_fd;
    SetupSender(&sock_fd);
    printf("Starting stream..");
    
    uint32_t packet_number = 0;
    uint16_t audio_buffer[ChunkBytes];
    int done=0;
    while(!done)
    {
        size_t bytes_read=fread(audio_buffer,sizeof(uint16_t),ChunkBytes,audio_file); 
        if(bytes_read == 0) {
            printf("End of file reached, stopping stream.\n");
            done = 1;
            break;
        }

        AudioPacket* packet = create_audio_packet(packet_number, audio_buffer,bytes_read);
        if (!packet) {
            fprintf(stderr, "Failed to create audio packet\n");
            continue;
        }
        size_t packet_size = sizeof(AudioPacket) + (bytes_read * sizeof(uint16_t));
        SendData(&sock_fd, packet,packet_size);
        
        free(packet);
        packet_number++;
        usleep(20000);     
    }

    printf("Audio streaming complete. Sent %u packets.\n", packet_number);
    
    close(sock_fd);
    return;

}



void ReceiveAudio(const char *ServerIP)
{
    int sock_fd;
    AudioBuffer buffer;
    init_circular_buffer(&buffer);
    char msg_buffer[1024];
    FILE *output_file = fopen("output.raw", "wb");
    if (!output_file) {
        perror("Failed to open output file");
        return;
    }

    SetupReceiver(ServerIP, &sock_fd);
    memset(buffer.packets, 0, sizeof(buffer.packets));
    buffer.next_expected_seq = 0;
  

    printf("Waiting for stream confirmation...\n");

    int flag=ReceiveData(&sock_fd,msg_buffer);
    if(flag==1)
   {
        while(1)
        {
        int res=ReceiveBufferPacket(sock_fd,&buffer);
        if(res>0)
        {
            AudioPacket* packet;
              while ((packet = GetNextPacket(&buffer)) != NULL) {
            fwrite(packet->AudioDataPCM, sizeof(uint16_t), ChunkBytes, output_file);
            free(packet);
        }
        }

   }

}

}