#include "common.h"

OpusContext *opus_initialize() {
    OpusContext *context = malloc(sizeof(OpusContext));
    int streams, coupled_streams;
    unsigned char mapping[CHANNELS];

    context->encoder = opus_multistream_surround_encoder_create(
        SAMPLE_RATE, CHANNELS, 1, &streams, &coupled_streams, mapping, 
        OPUS_APPLICATION_AUDIO, &context->error
    );

    context->decoder = opus_multistream_decoder_create(
        SAMPLE_RATE, CHANNELS, streams, coupled_streams, mapping, &context->error
    );

    return context;
}

int opus_encode_audio(OpusContext *context, opus_int16 *pcm, uint8_t *compressed_data) {
    return opus_multistream_encode(
        context->encoder, pcm, FRAMES_PER_BUFFER, compressed_data, MAX_OPUS_PACKET_SIZE
    );
}

int opus_decode_audio(OpusContext *context, uint8_t *compressed_data, int size, int16_t *pcm) {
    return opus_multistream_decode(
        context->decoder, compressed_data, size, pcm, FRAMES_PER_BUFFER, 0
    );
}

void opus_cleanup(OpusContext *ctx) {
    if (ctx->encoder) opus_multistream_encoder_destroy(ctx->encoder);
    if (ctx->decoder) opus_multistream_decoder_destroy(ctx->decoder);
    free(ctx);
}