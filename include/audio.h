#ifndef AUDIO_H
#define AUDIO_H

#include <stdio.h>
#include <stdint.h>

// Sample Rate: 44.1k
// Sample Format: 16-bit
// Channels: 2
// Chunk Duration: 25ms
#define ChunkBytes 4408
typedef struct AudioPacket{
    uint32_t PacketNumber;
    uint16_t AudioDataPCM[ChunkBytes];
    uint64_t timestamp_usec;
} AudioPacket;

#endif