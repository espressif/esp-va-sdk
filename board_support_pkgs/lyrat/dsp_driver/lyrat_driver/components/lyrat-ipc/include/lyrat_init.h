// Copyright 2018 Espressif Systems (Shanghai) PTE LTD
// All rights reserved.

#include <stdlib.h>

void lyrat_stop_capture();
void lyrat_start_capture();
int lyrat_stream_audio(uint8_t *buffer, int size, int wait);
void lyrat_mic_mute();
void lyrat_mic_unmute();
void lyrat_init();
