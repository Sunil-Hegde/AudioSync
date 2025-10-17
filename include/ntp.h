#ifndef NTP_H
#define NTP_H

#include "common.h"

#ifdef __APPLE__
#include <libkern/OSByteOrder.h>
#define htobe64(x) OSSwapHostToBigInt64(x)
#define be64toh(x) OSSwapBigToHostInt64(x)
#endif
#ifdef __linux__
#include <endian.h>
#endif
#ifdef _WINDOWS
  static inline uint64_t swap64(uint64_t x) {
      return ((x & 0x00000000000000FFULL) << 56) |
             ((x & 0x000000000000FF00ULL) << 40) |
             ((x & 0x0000000000FF0000ULL) << 24) |
             ((x & 0x00000000FF000000ULL) << 8)  |
             ((x & 0x000000FF00000000ULL) >> 8)  |
             ((x & 0x0000FF0000000000ULL) >> 24) |
             ((x & 0x00FF000000000000ULL) >> 40) |
             ((x & 0xFF00000000000000ULL) >> 56);
  }

  #define htobe64(x) swap64(x)
  #define be64toh(x) swap64(x)
#endif

typedef struct {
    uint64_t server_time;
    uint64_t client_time;
    int64_t  client_offset;
    uint32_t seq;
} SyncPacket;

void get_server_time(SyncPacket *sync_packet, uint32_t seq);
void get_client_time(SyncPacket *sync_packet);
uint64_t get_timestamp_ns(void);
#endif