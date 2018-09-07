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
#include <stdio.h>
#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>

#include <esp_err.h>
#include <esp_log.h>
#include <esp_audio_mem.h>
#include <hollow_stream.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

static const char *TAG = "HOLLOW_STREAM";

static esp_err_t hollow_stream_parse_config(void *base_stream)
{
    return ESP_OK;
}

static void hollow_stream_reset_config(void *base_stream)
{
    return;
}

static ssize_t hollow_stream_read(void *s, void *buf, ssize_t len)
{
    return ESP_OK;
}

static void hollow_stream_on_event(void *base_stream, audio_stream_event_t event)
{
    return;
}

hollow_stream_t *hollow_stream_create(hollow_stream_config_t *hollow_stream_cfg)
{

    hollow_stream_t *hollow_stream = (hollow_stream_t *) calloc(1, sizeof(hollow_stream_t));
    if(hollow_stream == NULL) {
        ESP_LOGE(TAG, "Failed to create holow stream");
        return NULL;
    }

    hollow_stream->base.cfg.task_stack_size = hollow_stream_cfg->hollow_stream_stack_sz;
    hollow_stream->base.cfg.task_priority = hollow_stream_cfg->hollow_stream_task_priority;
    hollow_stream->base.cfg.buf_size = hollow_stream_cfg->hollow_stream_buf_size;
    hollow_stream->base.cfg.derived_write = hollow_stream_cfg->hollow_stream_write_cb;

    hollow_stream->base.identifier = STREAM_TYPE_I2S;
    hollow_stream->base.type = STREAM_TYPE_WRITER;
    
    hollow_stream->base.cfg.w.input_wait = portMAX_DELAY;
    hollow_stream->base.cfg.w.output_wait = portMAX_DELAY;
    
    hollow_stream->base.cfg.derived_context_init = hollow_stream_parse_config;
    hollow_stream->base.cfg.derived_context_cleanup = hollow_stream_reset_config;
    hollow_stream->base.cfg.derived_read = hollow_stream_read;
    hollow_stream->base.cfg.derived_on_event = hollow_stream_on_event;

    return hollow_stream;
}

void hollow_stream_destroy(hollow_stream_t *hollow_stream)
{
    memset(hollow_stream, 0, sizeof(hollow_stream_t));
}
