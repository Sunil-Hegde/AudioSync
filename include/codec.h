#ifndef CODEC_H
#define CODEC_H
#include "common.h"
#include <opus/opus.h>

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