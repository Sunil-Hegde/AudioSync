#include "common.h"

OpusContext *opus_initialize(){
    OpusContext *context = malloc(sizeof(OpusContext));

    context->encoder = opus_encoder_create (
                        SAMPLE_RATE,
                        CHANNELS,
                        OPUS_APPLICATION_AUDIO,
                        &context->error);
    
    context->decoder = opus_decoder_create (
                        SAMPLE_RATE,
                        CHANNELS,
                        &context->error);
    
    opus_encoder_ctl(context->encoder, OPUS_SET_BITRATE(128000));
    return context;
}

int opus_encode_audio(
    OpusContext *context, 
    opus_int16 *pcm,
    uint8_t *compressed_data
){
    int compressed_size = opus_encode(
                            context->encoder,
                            pcm,
                            FRAMES_PER_BUFFER,
                            compressed_data,
                            MAX_OPUS_PACKET_SIZE);
    return compressed_size;
}

int opus_decode_audio(
    OpusContext *context,
    uint8_t *compressed_data,
    int compressed_size,
    int16_t *pcm
){
    int frame_size = opus_decode(
                              context->decoder,
                              compressed_data,
                              compressed_size,
                              pcm,
                              FRAMES_PER_BUFFER,
                              0);
    return frame_size;
}

void opus_cleanup(OpusContext *ctx) {
    if (ctx) {
        if (ctx->encoder) opus_encoder_destroy(ctx->encoder);
        if (ctx->decoder) opus_decoder_destroy(ctx->decoder);
        free(ctx);
    }
}