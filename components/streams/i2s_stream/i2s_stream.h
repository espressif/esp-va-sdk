/*
 * ESPRESSIF MIT License
 *
 * Copyright (c) 2018 <ESPRESSIF SYSTEMS (SHANGHAI) PTE LTD>
 *
 * Permission is hereby granted for use on all ESPRESSIF SYSTEMS products, in which case,
 * it is free of charge, to any person obtaining a copy of this software and associated
 * documentation files (the "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the Software is furnished
 * to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#ifndef _I2S_STREAM_H_
#define _I2S_STREAM_H_

#include <audio_stream.h>
#include <sys/types.h>
#include <driver/i2s.h>
#include <media_hal.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct i2s_stream_config {
    uint8_t i2s_num;
    i2s_config_t i2s_config;
    media_hal_t *media_hal_cfg;
} i2s_stream_config_t;

typedef struct i2s_stream {
    audio_stream_t base;
    i2s_stream_config_t cfg;
    void *i2s_resample_callback_arg;
    ssize_t (*i2s_resample_callback)(void *h, void *stream, void *buf, ssize_t len);
    /* Private members */
    size_t total_bytes;
    int _fill_zero_count;
} i2s_stream_t;

i2s_stream_t *i2s_reader_stream_create(i2s_stream_config_t *cfg);
i2s_stream_t *i2s_writer_stream_create(i2s_stream_config_t *cfg);

esp_err_t i2s_stream_destroy(i2s_stream_t *stream);
void i2s_stream_set_stack_size(i2s_stream_t *stream, ssize_t stack_size);

void i2s_stream_writer_disable(i2s_stream_t *stream);
void i2s_stream_writer_enable(i2s_stream_t *stream);

#define I2S_STREAM_BUFFER_SIZE        (512)
#define I2S_STREAM_TASK_STACK_SIZE    2200
#define I2S_STREAM_TASK_PRIORITY     5

#ifdef __cplusplus
}
#endif

#endif /* _I2S_STREAM_H_ */
