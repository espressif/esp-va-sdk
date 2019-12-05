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
#ifndef _AUDIO_CODEC_H_
#define _AUDIO_CODEC_H_

#include <esp_err.h>
#include <sys/types.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/task.h>
#include <audio_common.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CODEC_BASE(x) (&x->base)

typedef enum {
    CODEC_TYPE_MP3_DECODER = 1,
    CODEC_TYPE_AAC_DECODER,
    CODEC_TYPE_OPUS_DECODER,
    CODEC_TYPE_AMR_DECODER,
    CODEC_TYPE_WAV_DECODER,
    CODEC_TYPE_AMR_ENCODER,
    CODEC_TYPE_FLAC_ENCODER,
    CODEC_TYPE_OPUS_ENCODER
} audio_codec_type_t;

/* Audio type */
typedef enum {
    AUDIO_TYPE_UNKNOWN,
    AUDIO_TYPE_WAV,
    AUDIO_TYPE_AMRNB,
    AUDIO_TYPE_AMRWB,
    AUDIO_TYPE_M4A,
    AUDIO_TYPE_AAC,
    AUDIO_TYPE_TSAAC
} audio_type_t;

typedef enum {
    CODEC_STATE_INIT = 1,
    CODEC_STATE_RUNNING,
    CODEC_STATE_STOPPED,
    CODEC_STATE_PAUSED,
    CODEC_STATE_DESTROYED,
} audio_codec_state_t;

enum {
    CODEC_FAIL = ESP_FAIL,
    CODEC_OK   = 0,
    CODEC_DONE = 1,
};

typedef struct audio_codec_audio_info {
    int sampling_freq;
    int channels;
    int bits;
} audio_codec_audio_info_t;

#define CODEC_EVENT_START_NUM 32

typedef enum {
    CODEC_EVENT_STARTED = CODEC_EVENT_START_NUM,
    CODEC_EVENT_STOPPED,
    CODEC_EVENT_FAILED,
    CODEC_EVENT_PAUSED,
    CODEC_EVENT_DESTROYED,
    CODEC_EVENT_SET_FREQ,
} audio_codec_event_t;

typedef struct audio_codec audio_codec_t;

/* To be filled by the derived codec implementation */
typedef struct audio_codec_cfg {
    uint32_t task_stack_size;
    int task_priority;

    esp_err_t (*codec_open)(audio_codec_t *codec);
    esp_err_t (*codec_process)(audio_codec_t *codec);
    esp_err_t (*codec_stop)(audio_codec_t *codec);
    esp_err_t (*codec_close)(audio_codec_t *codec);

    uint32_t input_wait_ticks;
    uint32_t output_wait_ticks;
} audio_codec_cfg_t;

struct audio_codec {
    audio_codec_cfg_t cfg;

    const char *label;
    audio_codec_type_t identifier;

    /* To be filled by the higher layer application */
    audio_io_fn_arg_t codec_input;
    audio_io_fn_arg_t codec_output;
    audio_event_fn_arg_t event_func;

    TaskHandle_t thread;
    SemaphoreHandle_t ctrl_sem;
    audio_codec_state_t state;

    /* Control variables */
    uint8_t _run: 1;
    uint8_t _pause: 1;
    uint8_t _destroy: 1;
};

/* API for derived codecs */
void audio_codec_generate_event(audio_codec_t *codec, audio_codec_event_t event, void *data);

/* API for higher layer players or apps */
esp_err_t audio_codec_init(audio_codec_t *codec, const char *label, audio_io_fn_arg_t *codec_input,
                            audio_io_fn_arg_t *codec_output, audio_event_fn_arg_t *event_func);

esp_err_t audio_codec_modify_input_cb(audio_codec_t *codec, audio_io_fn_arg_t *codec_input);
audio_codec_type_t audio_codec_get_identifier(audio_codec_t *codec);

esp_err_t audio_codec_start(audio_codec_t *codec);
esp_err_t audio_codec_stop(audio_codec_t *codec);
esp_err_t audio_codec_pause(audio_codec_t *codec);
esp_err_t audio_codec_resume(audio_codec_t *codec);
esp_err_t audio_codec_destroy(audio_codec_t *codec);

#ifdef __cplusplus
}
#endif


#endif /* _AUDIO_CODEC_H_ */
