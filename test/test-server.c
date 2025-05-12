#define _POSIX_C_SOURCE 200112L

#include<stdio.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<netdb.h>
#include<string.h>
#include<stdlib.h>
#include<arpa/inet.h>
#include<netinet/in.h>

char *getIP(char *domainName){
    int status;
    struct addrinfo hints;
    struct addrinfo *service;
    static char ipstr[INET6_ADDRSTRLEN];

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if((status = getaddrinfo(domainName, NULL, &hints, &service)) != 0){
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
        exit(1);
    }
    struct sockaddr_in *ipv4 = (struct sockaddr_in *)service->ai_addr;
    inet_ntop(service->ai_family, (void *)&ipv4->sin_addr, ipstr, sizeof ipstr);
    freeaddrinfo(service);

    return ipstr;
}

int main(){
    struct addrinfo server_description, *server;
    int sock_fd, new_fd;
    struct sockaddr_storage receiving_addr;
    socklen_t receiving_address_size = sizeof receiving_addr;
    char server_ip[INET6_ADDRSTRLEN];
    char client_ip[INET6_ADDRSTRLEN];

    memset(&server_description, 0, sizeof server_description);
    server_description.ai_family = AF_INET;
    server_description.ai_socktype = SOCK_STREAM;
    server_description.ai_flags = AI_PASSIVE;

    getaddrinfo(NULL, "8080", &server_description, &server);

    sock_fd = socket(server->ai_family, server->ai_socktype, server->ai_protocol);
    inet_ntop(AF_INET,
        &((struct sockaddr_in *)server->ai_addr)->sin_addr,
        server_ip,
        sizeof server_ip);
    bind(sock_fd, server->ai_addr, server->ai_addrlen);
    listen(sock_fd, 5);
    printf("Server running at IP: %s and port 8080. Waiting for a clients to connect...\n", server_ip);

    while(1){
        new_fd = accept(sock_fd, (struct sockaddr *)&receiving_addr, &receiving_address_size);
        struct sockaddr_in *client_addr = (struct sockaddr_in *)&receiving_addr;
        inet_ntop(AF_INET, &client_addr->sin_addr, client_ip, sizeof client_ip);
        printf("Connected to client: %s\n", client_ip);

        char *message = "Hello from server";
        send(new_fd, message, strlen(message), 0);

        char buffer[1024];
        int bytes_received = recv(new_fd, &buffer, sizeof buffer - 1, 0);
        buffer[bytes_received] = '\0';
        printf("Requested IP for domain: %s\n", buffer);

        char resolvedIP[INET6_ADDRSTRLEN];
        strncpy(resolvedIP, getIP(buffer), sizeof(resolvedIP) - 1);
        resolvedIP[sizeof(resolvedIP) - 1] = '\0'; 
        
        printf("IP Address: %s\n", resolvedIP);
        send(new_fd, resolvedIP, strlen(resolvedIP), 0);
    }

    return 0;
}