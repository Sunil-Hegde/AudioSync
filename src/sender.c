#include "include/network.h"

int main(){
    int sock_fd, new_client_fd;
    SetupSender(&sock_fd);
    char *message = "Hello from server";

    while(1){
        SendData(&sock_fd, &new_client_fd, message);
        printf("Message sent!");
    }
    return 0;
}