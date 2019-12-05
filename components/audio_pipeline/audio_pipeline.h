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
#ifndef __AUDIO_PIPELINE_H__
#define __AUDIO_PIPELINE_H__

#include <esp_err.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <rom/queue.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <audio_stream.h>
#include <audio_codec.h>
#include <audio_common.h>
#include <ringbuf.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Default rinbuffer sizes */
#define RINGBUF1_DEFAULT_SIZE (8 * 1024)
#define RINGBUF2_DEFAULT_SIZE (8 * 1024)

/** Private members */
typedef enum {
    AUDIO_PIPE_INITED = 1,
    AUDIO_PIPE_STARTED,
    AUDIO_PIPE_STOPPED,
    AUDIO_PIPE_PAUSED,
    AUDIO_PIPE_RESUMED,
    AUDIO_PIPE_CHANGE_FREQ,
} audio_pipe_event_t;

typedef audio_pipe_event_t audio_pipe_state_t;

typedef enum {
    STREAM_BLOCK = 1,
    CODEC_BLOCK,
    CUSTOM_BLOCK,
} block_type_t;

typedef struct audio_pipe_block {
    block_type_t btype;
    void *block_cfg;
    ringbuf_t *rb;
    size_t rb_size;
    STAILQ_ENTRY(audio_pipe_block) next;
} audio_pipe_block_t;

typedef struct {
    const char *name;
    audio_pipe_state_t state;
    audio_pipe_state_t old_state;
    int cnt;
    audio_event_fn_arg_t event_func;
    xSemaphoreHandle lock;
    STAILQ_HEAD( , audio_pipe_block) pb;
} audio_pipe_t;

#define lock(x) \
            xSemaphoreTake(x, portMAX_DELAY)
#define unlock(x) \
            xSemaphoreGive(x)

/** Destroy audio pipeline
 *
 * @param[in] p Pipeline handle returned by \ref audio_pipe_create
 * @return ESP_OK or ESP_FAIL
 */
esp_err_t audio_pipe_destroy(audio_pipe_t *p);

/** Create audio player
 *
 * Create audio pipeline with input and output streams (codec is optional).
 */
audio_pipe_t *audio_pipe_create(const char *name, audio_stream_t *istream, size_t rb1_size,
                                audio_codec_t *codec, size_t rb2_size, audio_stream_t *ostream);

/** Create audio player with input callback
 *
 * Create audio pipeline with input callback (user defined).
 */
audio_pipe_t *audio_pipe_create_with_input_cb(const char *name, audio_io_fn_arg_t *io_cb,
        audio_codec_t *codec, size_t rb2_size, audio_stream_t *ostream);

/** Register event handler with pipeling
 *
 * @param[in] p Pipeline handle
 * @param[in] cb event handler callback
 * @return ESP_OK or ESP_FAIL
 */
esp_err_t audio_pipe_register_event_cb(audio_pipe_t *p, audio_event_fn_arg_t *event_func);

/** Get input stream type from pipeline */
audio_stream_identifier_t audio_pipe_get_ip_stream_type(audio_pipe_t *p);

/** Get input codec type from pipeline */
audio_codec_type_t audio_pipe_get_codec_type(audio_pipe_t *p);

/** Set/update input stream/callback with new_stream in pipeline
 *
 * Pipeline should either be in inited or stopped state for this API to work. Old stream (if any) in pipeline
 * will be destroyed and new stream will be initialzed and plugged in.
 */
esp_err_t audio_pipe_set_input_stream(audio_pipe_t *pipe, audio_stream_t *new_stream);

/** Set/update codec with new_codec in pipeline (if multiple codecs, then primary would be updated)
 *
 * Pipeline should either be in inited or stopped state for this API to work. Old codec (if any) in pipeline
 * will be destroyed and new codec will be initialized and plugged in.
 */
esp_err_t audio_pipe_set_codec(audio_pipe_t *pipe, audio_codec_t *new_codec);

/** Set/update input stream/callback with io_cb
 *
 * Pipeline should either be in inited or stopped state for this API to work. Old stream (if any) in pipeline
 * will be destroyed and callback will be plugged in.
 */
esp_err_t audio_pipe_set_input_cb(audio_pipe_t *pipe, audio_io_fn_arg_t *io_cb);

/* Asynchronous control APIs, waiting for appropriate event is required */
esp_err_t audio_pipe_start(audio_pipe_t *t);
esp_err_t audio_pipe_stop(audio_pipe_t *t);
esp_err_t audio_pipe_pause(audio_pipe_t *t);
esp_err_t audio_pipe_resume(audio_pipe_t *t);

#ifdef __cplusplus
}
#endif

#endif /* __AUDIO_PIPELINE_H__ */
