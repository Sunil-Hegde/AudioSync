#include "include/network.h"

int main(){
    char ServerIP[INET6_ADDRSTRLEN];
    printf("Enter server IP address: ");
    scanf("%s", ServerIP);
    
    int sock_fd;
    char buffer[1024];

    SetupReceiver(ServerIP, &sock_fd);
    ReceiveData(&sock_fd, buffer);
    return 0;
}