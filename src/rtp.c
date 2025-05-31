#include "rtp.h"
#include <arpa/inet.h>

void pack_rtp_header(RTPHeader *header, uint8_t *buffer) {
    buffer[0] = (header->version << 6) | 
                (header->padding << 5) | 
                (header->extension << 4) | 
                header->csrc_count;
    
    buffer[1] = (header->marker << 7) | header->payload_type;
    
    buffer[2] = (header->sequence_number >> 8) & 0xFF;
    buffer[3] = header->sequence_number & 0xFF;
    
    buffer[4] = (header->timestamp >> 24) & 0xFF;
    buffer[5] = (header->timestamp >> 16) & 0xFF;
    buffer[6] = (header->timestamp >> 8) & 0xFF;
    buffer[7] = header->timestamp & 0xFF;
    
    buffer[8] = (header->ssrc >> 24) & 0xFF;
    buffer[9] = (header->ssrc >> 16) & 0xFF;
    buffer[10] = (header->ssrc >> 8) & 0xFF;
    buffer[11] = header->ssrc & 0xFF;
}

void unpack_rtp_header(uint8_t *buffer, RTPHeader *header) {
    header->version = (buffer[0] >> 6) & 0x03;
    header->padding = (buffer[0] >> 5) & 0x01;
    header->extension = (buffer[0] >> 4) & 0x01;
    header->csrc_count = buffer[0] & 0x0F;
    
    header->marker = (buffer[1] >> 7) & 0x01;
    header->payload_type = buffer[1] & 0x7F;
    
    header->sequence_number = (buffer[2] << 8) | buffer[3];
    
    header->timestamp = (buffer[4] << 24) | (buffer[5] << 16) | 
                        (buffer[6] << 8) | buffer[7];
    
    header->ssrc = (buffer[8] << 24) | (buffer[9] << 16) | 
                  (buffer[10] << 8) | buffer[11];
}