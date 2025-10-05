#include "network.h"
#include "audio.h"
#include "ntp.h"
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>

int main() {
    AudioBuffer buffer;
    init_circular_buffer(&buffer);
    ReceiveAudio(&buffer);
    
    return 0;
}