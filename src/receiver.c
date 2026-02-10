#include "common.h"

int main(int argc, char *argv[]) {
    AudioBuffer buffer;
    int channel = 0;
    
    if (argc >= 2) {
        channel = atoi(argv[1]);
    }

    printf("Usage: %s [channel]\n", argv[0]);
    printf("Channels: 0=FL, 1=FR, 2=Center, 3=LFE, 4=SL, 5=SR\n\n");

    init_circular_buffer(&buffer);
    ReceiveAudio(&buffer, channel);
    
    return 0;
}