#include "include/network.h"
#include <unistd.h>

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

void SetupReceiver(char *ServerIP, int *sock_fd){
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
