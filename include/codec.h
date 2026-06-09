/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2025 Sunil Hegde & Mythili Shetty
 */

#ifndef CODEC_H
#define CODEC_H
#include "common.h"

// Chunk Duration: 20ms
#define PACKET_INTERVAL_NS 20000000ULL
// Sample Format: 16-bit
#define SAMPLE_RATE 48000
#define CHANNELS 2

#define ChunkBytes 3840
#define FRAMES_PER_BUFFER (ChunkBytes / (CHANNELS * sizeof(uint16_t)))

#define MAX_BUFFER_SIZE 50
#define MAX_OUT_OF_ORDER 10

#define PCM_DATA_SIZE_IN_BYTES ChunkBytes 
#define PCM_DATA_SIZE_IN_ELEMENTS (PCM_DATA_SIZE_IN_BYTES / sizeof(uint16_t))
#define MAX_OPUS_PACKET_SIZE 4000

typedef struct {
    OpusEncoder *encoder;
    OpusDecoder *decoder;
    int error;
} OpusContext;

OpusContext *opus_initialize();
int opus_encode_audio(
    OpusContext *context, 
    opus_int16 *pcm,
    uint8_t *compressed_data
);
int opus_decode_audio(
    OpusContext *context,
    uint8_t *compressed_data,
    int compressed_size,
    int16_t *pcm
);
void opus_cleanup(OpusContext *ctx);
#endif