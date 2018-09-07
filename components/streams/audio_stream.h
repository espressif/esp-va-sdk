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
#ifndef _AUDIO_STREAM_H_
#define _AUDIO_STREAM_H_

#include <esp_err.h>
#include <sys/types.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/task.h>
#include <audio_common.h>

#ifdef __cplusplus
extern "C" {
#endif

#define STREAM_BASE(x) (&x->base)

typedef enum {
    STREAM_TYPE_I2S = 1,
    STREAM_TYPE_FS,
    STREAM_TYPE_HTTP,
    STREAM_TYPE_CALLBACK,
} audio_stream_identifier_t;

typedef enum {
    STREAM_STATE_INIT = 1,
    STREAM_STATE_RUNNING,
    STREAM_STATE_STOPPED,
    STREAM_STATE_PAUSED,
    STREAM_STATE_DESTROYED,
} audio_stream_state_t;

typedef enum {
    STREAM_TYPE_READER = 1, /* Reader reads from file and outputs to stream_output */
    STREAM_TYPE_WRITER, /* Writer reads using stream_input and outputs to a file */
} audio_stream_type_t;

typedef enum {
    STREAM_EVENT_STARTED = 2,
    STREAM_EVENT_STOPPED,
    STREAM_EVENT_PAUSED,
    STREAM_EVENT_DESTROYED,
    STREAM_EVENT_CUSTOM_DATA,
    STREAM_EVENT_FAILED,
    STREAM_EVENT_USER_THRESHOLD,              //enum event which can be used by apps for defining their own enums variables.
} audio_stream_event_t;

typedef struct audio_stream audio_stream_t;

/* This configuration is to be filled by the derived stream */
typedef struct audio_stream_cfg {
    uint32_t task_stack_size;
    int task_priority;
    ssize_t buf_size;
    union {
        uint32_t input_wait; /* Valid for STREAM_TYPE_WRITER */
        uint32_t output_wait; /* Valid for STREAM_TYPE_READER */
    } w;

    esp_err_t (*derived_context_init)(void *stream);
    ssize_t (*derived_read)(void *stream, void *buf, ssize_t len);
    ssize_t (*derived_write)(void *stream, void *buf, ssize_t len);
    void (*derived_context_cleanup)(void *stream);
    void (*derived_on_event)(void *stream, audio_stream_event_t event);

} audio_stream_cfg_t;

typedef struct audio_stream {
    audio_stream_cfg_t cfg;

    const char *label;
    audio_event_fn_arg_t event_func;
    audio_stream_type_t type;
    audio_stream_identifier_t identifier;

    /* These operations are to be filled by end-app or layer above derived streams */
    union {
        /* stream_input valid for STREAM_TYPE_WRITER */
        audio_io_fn_arg_t stream_input;
        /* stream_output valid for STREAM_TYPE_READER */
        audio_io_fn_arg_t stream_output;
    } op;

    TaskHandle_t thread;
    void *buf;
    SemaphoreHandle_t ctrl_sem;
    audio_stream_state_t state;

    /* Control variables */
    uint8_t _run: 1;
    uint8_t _pause: 1;
    uint8_t _destroy: 1;

} audio_stream_t;

esp_err_t audio_stream_init(audio_stream_t *stream, const char *label, audio_io_fn_arg_t *stream_io, audio_event_fn_arg_t *event_func);

audio_stream_identifier_t audio_stream_get_identifier(audio_stream_t *stream);

esp_err_t audio_stream_start(audio_stream_t *stream);
esp_err_t audio_stream_stop(audio_stream_t *stream);
esp_err_t audio_stream_pause(audio_stream_t *stream);
esp_err_t audio_stream_resume(audio_stream_t *stream);

esp_err_t audio_stream_destroy(audio_stream_t *stream);
audio_stream_state_t audio_stream_get_state(audio_stream_t *stream);

#ifdef __cplusplus
}
#endif


#endif
