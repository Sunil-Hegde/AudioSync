#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#ifndef _WINDOWS
    #include <unistd.h>
    #include <sys/types.h>
    #include <sys/socket.h>
    #include <arpa/inet.h>
    #include <netinet/in.h>
#else
    #include <winsock2.h>
    #include <windows.h>
    #include <ws2tcpip.h>
#endif

#include <time.h>
#include <portaudio.h>
#include <opus/opus.h>
#include "network.h"
#include "audio.h"
#include "ntp.h"
#include "codec.h"

#endif // COMMON_H