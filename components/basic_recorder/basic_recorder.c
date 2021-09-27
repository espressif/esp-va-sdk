// Copyright 2018 Espressif Systems (Shanghai) PTE LTD
// All rights reserved.

#include <string.h>
#include <esp_audio_mem.h>
#include <esp_log.h>
#include <esp_err.h>
#include <abstract_rb_utils.h>

#include "basic_recorder.h"

static const char *TAG = "[basic_recorder]";

struct basic_recorder {
    audio_codec_type_t codec_type; /* Current active codec */
    struct audio_codec_list { /* Make sure to access this from CODEC_TYPE_DEC_MAX */
        audio_codec_t *base;
    } codec[CODEC_TYPE_MAX - CODEC_TYPE_DEC_MAX]; /* To not include decoder types */

    read_func_cb_t codec_read_cb;
    write_func_cb_t codec_write_cb;
    event_func_cb_t recorder_event_cb;

    void *codec_read_cb_data;
    void *codec_write_cb_data;
    void *recorder_event_cb_data;

    bool is_codec_stopped;
    bool is_running;
};

static ssize_t basic_recorder_codec_read_cb(void *arg, void *data, int len, unsigned int wait)
{
    ssize_t ret;
    struct basic_recorder *b = (struct basic_recorder *)arg;

    /* Not worrying for anchors for now, as this is done with callback */
    ret = (*b->codec_read_cb)(b->codec_read_cb_data, data, len, wait);
    return ret;
}

static ssize_t basic_recorder_codec_write_cb(void *arg, void *data, int len, unsigned int wait)
{
    ssize_t ret = len;
    struct basic_recorder *b = (struct basic_recorder *)arg;
    if (len > 0) {
        /* Not worrying for anchors for now, as this is done with callback */
        ret = (*b->codec_write_cb)(b->codec_write_cb_data, data, len, wait);
    }
    return ret;
}

esp_err_t basic_recorder_record(basic_recorder_handle_t handle, basic_recorder_record_config_t *record_cfg)
{
    if (handle == NULL) {
        ESP_LOGE(TAG, "Handle is null");
        return ESP_FAIL;
    }

    struct basic_recorder *b = (struct basic_recorder *) handle;
    basic_recorder_stop(handle);

    b->codec_type = record_cfg->encoder_type;
    b->recorder_event_cb = record_cfg->event_cb;
    b->codec_read_cb = record_cfg->read_cb;
    b->codec_write_cb = record_cfg->write_cb;
    b->recorder_event_cb_data = record_cfg->event_cb_data;
    b->codec_read_cb_data = record_cfg->read_cb_data;
    b->codec_write_cb_data = record_cfg->write_cb_data;

    b->is_codec_stopped = true;
    b->is_running = true;

    audio_codec_start(b->codec[b->codec_type - CODEC_TYPE_DEC_MAX].base);

    return ESP_OK;
}


esp_err_t basic_recorder_stop(basic_recorder_handle_t handle)
{
    if (handle == NULL) {
        ESP_LOGE(TAG, "Handle is null");
        return ESP_FAIL;
    }
    struct basic_recorder *b = (struct basic_recorder *) handle;

    if (!b->is_running) {
        return ESP_FAIL;
    }

    audio_codec_stop(b->codec[b->codec_type - CODEC_TYPE_DEC_MAX].base);

    /* Wait for recorder to stop */
    while (!b->is_codec_stopped) {
        printf("Waiting for recorder to stop!\n");
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    b->is_running = false;
    /* Raise recorder stopped event */
    b->recorder_event_cb(NULL, RECORDER_EVENT_STOPPED);
    return ESP_OK;
}

static const char *basic_recorder_codec_get_event_str(audio_codec_event_t event)
{
    switch (event) {
    case CODEC_EVENT_STARTED:
        return "Codec Started";
        break;
    case CODEC_EVENT_STOPPED:
        return "Codec Stopped";
        break;
    case CODEC_EVENT_FAILED:
        return "Codec Failed";
        break;
    case CODEC_EVENT_PAUSED:
        return "Codec Paused";
        break;
    case CODEC_EVENT_DESTROYED:
        return "Codec Destroyed";
        break;
    default:
        break;
    }
    return "Invalid";
}

static const char *basic_recorder_stream_get_codec_str(audio_codec_type_t type)
{
    switch (type) {
    case CODEC_TYPE_AMR_ENCODER:
        return "amr_encoder";
    case CODEC_TYPE_FLAC_ENCODER:
        return "flac_encoder";
    case CODEC_TYPE_OPUS_ENCODER:
        return "opus_encoder";
    default:
        break;
    }
    return "invalid_encoder";
}

static esp_err_t basic_recorder_codec_event_cb(void *arg, int event, void *data)
{
    struct basic_recorder *b = (struct basic_recorder *) arg;
    printf("%s: Codec event: %s\n", TAG, basic_recorder_codec_get_event_str(event));

    switch(event) {
        case CODEC_EVENT_STARTED:
            b->is_codec_stopped = false;
            break;

        case CODEC_EVENT_STOPPED:
            b->is_codec_stopped = true;
            break;

        case CODEC_EVENT_FAILED:
            b->is_codec_stopped = true;
            b->recorder_event_cb(NULL, RECORDER_EVENT_FAILED);
            break;

        default:
            break;
    }
    return ESP_OK;
}

esp_err_t basic_recorder_add_codec(basic_recorder_handle_t handle, audio_codec_type_t type, audio_codec_t *base)
{
    if (handle == NULL) {
        ESP_LOGE(TAG, "Handle is null");
        return ESP_FAIL;
    }
    struct basic_recorder *b = (struct basic_recorder *)handle;
    if (b->codec[type - CODEC_TYPE_DEC_MAX].base) {
        ESP_LOGE(TAG, "audio_codec: %s already exists. Remove it first to change it.", basic_recorder_stream_get_codec_str(type));
        return ESP_FAIL;
    }

    b->codec[type - CODEC_TYPE_DEC_MAX].base = base;
    audio_io_fn_arg_t codec_read_fn = { .func = basic_recorder_codec_read_cb, .arg = b };
    audio_io_fn_arg_t codec_write_fn = { .func = basic_recorder_codec_write_cb, .arg = b };
    audio_event_fn_arg_t codec_event_fn = { .func = basic_recorder_codec_event_cb, .arg = b };

    if (audio_codec_init(b->codec[type - CODEC_TYPE_DEC_MAX].base, basic_recorder_stream_get_codec_str(type),
                         &codec_read_fn, &codec_write_fn, &codec_event_fn) != 0) {
        ESP_LOGE(TAG, "Error initializing audio_codec: %s", basic_recorder_stream_get_codec_str(type));
        return ESP_FAIL;
    }
    return ESP_OK;
}

esp_err_t basic_recorder_remove_codec(basic_recorder_handle_t handle, audio_codec_type_t type)
{
    if (handle == NULL) {
        ESP_LOGE(TAG, "Handle is null");
        return ESP_FAIL;
    }
    struct basic_recorder *b = (struct basic_recorder *)handle;
    if (b->codec[type - CODEC_TYPE_DEC_MAX].base) {
        audio_codec_destroy(b->codec[type - CODEC_TYPE_DEC_MAX].base);
        b->codec[type - CODEC_TYPE_DEC_MAX].base = NULL;
        return ESP_OK;
    }
    return ESP_FAIL;
}

basic_recorder_handle_t basic_recorder_create(basic_recorder_cfg_t *basic_recorder_cfg)
{
    struct basic_recorder *b = esp_audio_mem_calloc(1, sizeof (struct basic_recorder));

    if (!b) {
        ESP_LOGE(TAG, "basic_recorder calloc failed");
        return NULL;
    }

    return (basic_recorder_handle_t) b;
}

void basic_recorder_destroy(basic_recorder_handle_t handle)
{
    if (handle == NULL) {
        ESP_LOGE(TAG, "Handle is null");
        return;
    }

    struct basic_recorder *b = (struct basic_recorder *) handle;

    /* Trigger recorder stop */
    if (b->is_running) {
        basic_recorder_stop((basic_recorder_handle_t) b);
    }

    /* Release all memory */
    free(handle);
}
