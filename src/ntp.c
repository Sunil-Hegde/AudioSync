#include "common.h"

#ifdef _WINDOWS
    struct timespec {
        time_t tv_sec;
        long   tv_nsec;
    };

    static inline int clock_gettime_monotonic(struct timespec *ts) {
        static LARGE_INTEGER freq;
        static int initialized = 0;
        LARGE_INTEGER count;
        if (!initialized) {
            QueryPerformanceFrequency(&freq);
            initialized = 1;
        }
        QueryPerformanceCounter(&count);
        ts->tv_sec = count.QuadPart / freq.QuadPart;
        ts->tv_nsec = (count.QuadPart % freq.QuadPart) * 1000000000 / freq.QuadPart;
        return 0;
    }

    #define clock_gettime(c, ts) clock_gettime_monotonic(ts)
#endif

void get_server_time(SyncPacket *sync_packet, uint32_t seq){
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    uint64_t ns = (uint64_t)ts.tv_sec * 1000000000ULL + ts.tv_nsec;

    sync_packet->server_time = htobe64(ns);
    sync_packet->seq = htonl(seq);
}

// Client: record local time and compute offset
void get_client_time(SyncPacket *sync_packet){
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    uint64_t client_ns = (uint64_t)ts.tv_sec * 1000000000ULL + ts.tv_nsec;

    uint64_t server_ns = be64toh(sync_packet->server_time); 
    sync_packet->client_time = htobe64(client_ns);
    sync_packet->client_offset = (int64_t)server_ns - (int64_t)client_ns;
}

uint64_t get_timestamp_ns(void){
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + ts.tv_nsec;
}
