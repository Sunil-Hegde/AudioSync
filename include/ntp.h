#pragma once
#include <stdint.h>
#include <time.h>
#include <arpa/inet.h>

#ifdef __APPLE__
#include <libkern/OSByteOrder.h>
#define htobe64(x) OSSwapHostToBigInt64(x)
#define be64toh(x) OSSwapBigToHostInt64(x)
#else
#include <endian.h>
#endif

typedef struct {
    uint64_t server_time;
    uint64_t client_time;
    int64_t  client_offset;
    uint32_t seq;
} SyncPacket;

void get_server_time(SyncPacket *sync_packet, uint32_t seq);
void get_client_time(SyncPacket *sync_packet);
