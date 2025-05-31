#ifndef RTP_H
#define RTP_H

#include <stdint.h>
#include <stddef.h>  // This was missing - needed for size_t

#define RTP_VERSION 2
#define RTP_PAYLOAD_TYPE_PCM 0  // PCM audio
#define RTP_HEADER_SIZE 12      // Size of the basic RTP header
#define RTP_MAX_PACKET_SIZE 1400   // Maximum packet size
#define RTP_MAX_PAYLOAD_SIZE (RTP_MAX_PACKET_SIZE - RTP_HEADER_SIZE)

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

typedef struct {
    RTPHeader header;
    uint8_t payload[RTP_MAX_PAYLOAD_SIZE];
    size_t payload_length;
} RTPPacket;

void create_rtp_packet(RTPPacket *packet, uint16_t seq_num, uint32_t timestamp, 
                      uint32_t ssrc, const uint8_t *audio_data, size_t data_length);
void serialize_rtp_packet(const RTPPacket *packet, uint8_t *buffer, size_t *total_length);
void deserialize_rtp_packet(const uint8_t *buffer, size_t buffer_length, RTPPacket *packet);

// Pack RTP header into network byte order
void pack_rtp_header(RTPHeader *header, uint8_t *buffer);

// Unpack RTP header from network byte order
void unpack_rtp_header(uint8_t *buffer, RTPHeader *header);

#endif // RTP_H