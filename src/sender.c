/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2025 Sunil Hegde & Mythili Shetty
 */

 #include "common.h"

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <audio_file.raw>\n", argv[0]);
        return 1;
    }

    FILE *audio_file = fopen(argv[1], "rb");
    if (!audio_file) {
        perror("Failed to open audio file");
        return 1;
    }
    
    printf("Starting audio streaming from file: %s\n", argv[1]);
    printf("Chunk size: %d Bytes\n", ChunkBytes);
    
    PacketSetupAndSend(audio_file);
    
    fclose(audio_file);
    return 0;
}