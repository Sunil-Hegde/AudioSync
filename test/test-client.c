#define _POSIX_C_SOURCE 200112L

#include<stdio.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netdb.h>
#include<string.h>
#include<stdlib.h>
#include<arpa/inet.h>
#include<netinet/in.h>

#define PORT "8080"

int main(int argc, char *argv[]){
    struct addrinfo server_description, *server;
    int status;
    char buffer[1024];
    char input[256];

    memset(&server_description, 0, sizeof server_description);
    server_description.ai_family = AF_INET;
    server_description.ai_socktype = SOCK_STREAM;
    getaddrinfo(argv[1], PORT, &server_description, &server);
    int sock_fd = socket(server->ai_family, server->ai_socktype, server->ai_protocol);

    printf("Connecting to server at IP: %s\n", argv[1]);

    status = connect(sock_fd, server->ai_addr, server->ai_addrlen);
    if(status != -1){
        printf("Connected to server!\n");
    } else {
        printf("Invalid IP\n");
    }
    int bytes_received = recv(sock_fd, &buffer, sizeof buffer - 1, 0);
    buffer[bytes_received] = '\0';
    printf("Message received from server: %s\n", buffer);

    printf("Enter the domain to resolve its IP: ");
    scanf("%s", input);

    send(sock_fd, input, strlen(input), 0);
    bytes_received = recv(sock_fd, &buffer, sizeof buffer - 1, 0);
    buffer[bytes_received] = '\0';
    
    printf("Resolved IP Address: %s\n", buffer);

    return 0;
}