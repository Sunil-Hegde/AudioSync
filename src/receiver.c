#include "network.h"
#include "audio.h"
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s <sender_ip> \n", argv[0]);
        return 1;
    }
    
    const char *server_ip = argv[1];
    AudioBuffer buffer;
    init_circular_buffer(&buffer);
    ReceiveAudio(server_ip, &buffer);
    
    return 0;
}