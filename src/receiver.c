#include "network.h"
#include "audio.h"
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s <sender_ip> <output_file.raw>\n", argv[0]);
        return 1;
    }
    
    const char *server_ip = argv[1];
    const char *output_filename = argv[2];
    
    // Use the consolidated function to receive audio
    ReceiveAudio(server_ip, output_filename);
    
    return 0;
}