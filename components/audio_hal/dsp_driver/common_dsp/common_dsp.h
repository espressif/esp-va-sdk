// Copyright 2018 Espressif Systems (Shanghai) PTE LTD
// All rights reserved.

#pragma once

#include <stdlib.h>

typedef struct common_dsp_config {
    int ring_buffer_size;
    int task_stack_size;
} common_dsp_config_t;

void common_dsp_stop_capture();
void common_dsp_start_capture();
int common_dsp_get_ww_len();
int common_dsp_stream_audio(uint8_t *buffer, int size, int wait);
void common_dsp_mic_mute();
void common_dsp_mic_unmute();
void common_dsp_configure(common_dsp_config_t *cfg);
void common_dsp_init(QueueHandle_t queue);
int common_dsp_write_mic_data(void *data, int len, uint32_t wait);
