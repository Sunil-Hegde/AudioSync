/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2025 Sunil Hegde & Mythili Shetty
 */

 #include "common.h"

int main() {
    AudioBuffer buffer;
    init_circular_buffer(&buffer);
    ReceiveAudio(&buffer);
    
    return 0;
}