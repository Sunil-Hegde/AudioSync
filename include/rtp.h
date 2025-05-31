#ifndef RTP_H
#define RTP_H

#include <stdint.h>

#define RTP_VERSION 2
#define RTP_PAYLOAD_TYPE_PCM 0  // PCM audio
#define RTP_HEADER_SIZE 12      // Size of the basic RTP header

typedef struct {
    uint8_t version:2;        // Protocol version
    uint8_t padding:1;        // Padding flag
    uint8_t extension:1;      // Extension flag
    uint8_t csrc_count:4;     // CSRC count
    uint8_t marker:1;         // Marker bit
    uint8_t payload_type:7;   // Payload type
    uint16_t sequence_number; // Sequence number
    uint32_t timestamp;       // Timestamp
    uint32_t ssrc;            // Synchronization source
} RTPHeader;

// Pack RTP header into network byte order
void pack_rtp_header(RTPHeader *header, uint8_t *buffer);

// Unpack RTP header from network byte order
void unpack_rtp_header(uint8_t *buffer, RTPHeader *header);

#endif // RTP_H