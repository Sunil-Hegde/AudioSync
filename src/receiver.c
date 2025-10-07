#include "common.h"

int main() {
    AudioBuffer buffer;
    init_circular_buffer(&buffer);
    ReceiveAudio(&buffer);
    
    return 0;
}