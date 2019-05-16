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
#include <i2s_stream.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <driver/i2s.h>
#include <audio_board.h>

#define I2STAG   "I2SStream"

static esp_err_t parse_i2s_config(void *base_stream)
{
    esp_err_t ret;
    i2s_stream_t *stream = (i2s_stream_t *) base_stream;
    i2s_stream_config_t *cfg = &stream->cfg;

    ret = i2s_driver_install(cfg->i2s_num, &cfg->i2s_config, 0, NULL);
    ESP_LOGI(I2STAG, ".......................Init i2s stream...............\n");
    if (ret != ESP_OK) {
        ESP_LOGE(I2STAG, "Error installing i2s driver for stream %s", stream->base.label);
    } else {
        i2s_pin_config_t pf_i2s_pin = {0};
        audio_board_i2s_pin_config(cfg->i2s_num, &pf_i2s_pin);
        i2s_set_pin(cfg->i2s_num, &pf_i2s_pin);
    }
    i2s_zero_dma_buffer(cfg->i2s_num);

    return ESP_OK;
}

static void reset_i2s_config(void *base_stream)
{
    i2s_stream_t *stream = (i2s_stream_t *) base_stream;
    i2s_stream_config_t *cfg = &stream->cfg;
    i2s_zero_dma_buffer(cfg->i2s_num);

    return;
}

static ssize_t i2s_stream_read(void *s, void *buf, ssize_t len)
{
    ssize_t ret;
    size_t bytes_read;
    i2s_stream_t *stream = (i2s_stream_t *) s;
    ret = i2s_read(stream->cfg.i2s_num, buf, len, &bytes_read, portMAX_DELAY);

    if (ret == ESP_OK) {
        return bytes_read;
    } else {
        return ret;
    }
}

static ssize_t i2s_stream_write(void *s, void *buf, ssize_t len)
{
    ssize_t ret = 0;
    i2s_stream_t *stream = (i2s_stream_t *) s;
    ret = stream->i2s_resample_callback(stream->i2s_resample_callback_arg, s, buf, len);

    return ret;
}

static void i2s_on_event(void *base_stream, audio_stream_event_t event)
{
    if (event == STREAM_EVENT_PAUSED || event == STREAM_EVENT_STOPPED || event == STREAM_EVENT_DESTROYED) {
        i2s_stream_t *stream = (i2s_stream_t *) base_stream;
        i2s_zero_dma_buffer(stream->cfg.i2s_num);
    }
}

static void i2s_stream_free(i2s_stream_t *stream)
{
    if (!stream) {
        return;
    }

    i2s_driver_uninstall(stream->cfg.i2s_num);
    free(stream);
}

void i2s_stream_set_stack_size(i2s_stream_t *stream, ssize_t stack_size)
{
   if (stream == NULL)
       return;
   stream->base.cfg.task_stack_size = stack_size;
}

static ssize_t null_write(void *s, void *buf, ssize_t len)
{
    return len;
}

void i2s_stream_writer_disable(i2s_stream_t *stream)
{
    stream->base.cfg.derived_write = null_write;
    i2s_zero_dma_buffer(stream->cfg.i2s_num);
}

void i2s_stream_writer_enable(i2s_stream_t *stream)
{
    stream->base.cfg.derived_write = i2s_stream_write;
}

static i2s_stream_t *i2s_stream_create(i2s_stream_config_t *cfg, audio_stream_type_t type)
{
    i2s_stream_t *stream = (i2s_stream_t *) calloc(1, sizeof(i2s_stream_t));
    configASSERT(stream);

    if (cfg) {
        memcpy(&stream->cfg, cfg, sizeof(i2s_stream_config_t));
    }

    stream->base.cfg.task_stack_size = I2S_STREAM_TASK_STACK_SIZE;
    stream->base.cfg.task_priority = I2S_STREAM_TASK_PRIORITY;
    stream->base.cfg.buf_size = I2S_STREAM_BUFFER_SIZE;

    stream->base.identifier = STREAM_TYPE_I2S;

    /* Set stream specific operations */
    stream->base.cfg.derived_context_init = parse_i2s_config;
    stream->base.cfg.derived_context_cleanup = reset_i2s_config;
    stream->base.cfg.derived_read = i2s_stream_read;
    stream->base.cfg.derived_write = i2s_stream_write;
    stream->base.cfg.derived_on_event = i2s_on_event;

    stream->base.cfg.w.input_wait = portMAX_DELAY;
    stream->base.cfg.w.output_wait = portMAX_DELAY;

    stream->base.type = type;

    return stream;
}

i2s_stream_t *i2s_reader_stream_create(i2s_stream_config_t *cfg)
{
    return i2s_stream_create(cfg, STREAM_TYPE_READER);
}

i2s_stream_t *i2s_writer_stream_create(i2s_stream_config_t *cfg)
{
    return i2s_stream_create(cfg, STREAM_TYPE_WRITER);
}

esp_err_t i2s_stream_destroy(i2s_stream_t *stream)
{
    i2s_stream_free(stream);
    return ESP_OK;
}
