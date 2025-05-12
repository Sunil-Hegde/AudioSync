#include "include/network.h"

void SetupSender(int *sock_fd){
    struct addrinfo server_description, *server;
    char server_ip[INET6_ADDRSTRLEN];

    memset(&server_description, 0, sizeof server_description);
    server_description.ai_family = AF_INET;
    server_description.ai_socktype = SOCK_STREAM;
    server_description.ai_flags = AI_PASSIVE;

    getaddrinfo(NULL, PORT, &server_description, &server);

    *sock_fd = socket(server->ai_family, server->ai_socktype, server->ai_protocol);
    inet_ntop(AF_INET, &((struct sockaddr_in *)server->ai_addr)->sin_addr, server_ip, sizeof server_ip);
    bind(*sock_fd, server->ai_addr, server->ai_addrlen);
    listen(*sock_fd, 5);
    printf("Server running at IP: %s and port %s.\nWaiting for clients to connect...\n", server_ip, PORT);
}

void SendData(int *sock_fd, int *new_sock_fd, char *message){
    struct sockaddr_storage receiving_addr;
    socklen_t receiving_address_size = sizeof receiving_addr;
    char client_ip[INET6_ADDRSTRLEN];

    *new_sock_fd = accept(*sock_fd, (struct sockaddr *)&receiving_addr, &receiving_address_size);
    struct sockaddr_in *client_addr = (struct sockaddr_in *)&receiving_addr;
    inet_ntop(AF_INET, &client_addr->sin_addr, client_ip, sizeof client_ip);
    printf("Clent connected! IP Address: %s\n", client_ip);

    send(*new_sock_fd, message, strlen(message), 0);
}

void SetupReceiver(char *ServerIP, int *sock_fd){
    struct addrinfo server_description, *server;
    int status;

    memset(&server_description, 0, sizeof server_description);
    server_description.ai_family = AF_INET;
    server_description.ai_socktype = SOCK_STREAM;
    getaddrinfo(ServerIP, PORT, &server_description, &server);
    *sock_fd = socket(server->ai_family, server->ai_socktype, server->ai_protocol);

    printf("Connecting to server at IP: %s\n", ServerIP);

    status = connect(*sock_fd, server->ai_addr, server->ai_addrlen);

    if(status != -1){
        printf("Connected to server!\n");
    } else {
        printf("Invalid IP\n");
    }
}

void ReceiveData(int *sock_fd, char *buffer){
    int bytes_received = recv(*sock_fd, buffer, 1023, 0);
    if (bytes_received > 0) {
        buffer[bytes_received] = '\0';
        printf("Message from server: %s\n", buffer);
    } else {
        perror("recv");
    }
}
