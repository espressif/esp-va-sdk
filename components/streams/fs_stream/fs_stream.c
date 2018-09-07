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
#include <fs_stream.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#define FSTAG   "FATFSStream"

static esp_err_t parse_fs_config(void *base_stream)
{
    fs_stream_t *stream = (fs_stream_t *) base_stream;

    if (strlen(stream->cfg.file_path) == 0) {
        return -1;
    }

    if (stream->base.type == STREAM_TYPE_WRITER) {
        stream->_file = fopen(stream->cfg.file_path, "w");
        if (stream->_file) {
            /* Recorder always creates a new file */
            if (fseek(stream->_file, 0, SEEK_SET) != 0) {
                ESP_LOGE(FSTAG, "Error seek file");
            }
        }
    } else {
        stream->_file = fopen(stream->cfg.file_path, "r");
        if (stream->_file) {
            struct stat st = {0};
            stat(stream->cfg.file_path, &st) ;
            stream->total_bytes = st.st_size;
            ESP_LOGI(FSTAG, "File size is %d byte", (int)st.st_size);
        }
    }
    if (stream->_file == NULL) {
        ESP_LOGE(FSTAG, "Error opening file %s", stream->cfg.file_path);
        return -3;
    }

    return ESP_OK;
}

static void reset_fs_config(void *base_stream)
{
    fs_stream_t *stream = (fs_stream_t *) base_stream;
    if (stream->_file) {
        fclose(stream->_file);
        stream->_file = NULL;
    }

    memset(stream->cfg.file_path, 0, sizeof(stream->cfg.file_path));

    return;
}
ssize_t fs_read(void *s, void *buf, ssize_t len)
{
    int ret;
    fs_stream_t *stream = (fs_stream_t *) s;
    ret = fread(buf, 1, len, stream->_file);
    if (ret == 0) {
        return -1;
    }
    return ret;
}

ssize_t fs_write(void *s, void *buf, ssize_t len)
{
    int ret;
    fs_stream_t *stream = (fs_stream_t *) s;
    ret = fwrite(buf, 1, len, stream->_file);
    if (ret == 0) {
        return -1;
    }
    return ret;
}
static void fs_stream_free(fs_stream_t *stream)
{
    if (!stream) {
        return;
    }

    free(stream);
}

void fs_stream_set_stack_size(fs_stream_t *stream, ssize_t stack_size)
{
   if (stream == NULL)
       return;
   stream->base.cfg.task_stack_size = stack_size;
}

static fs_stream_t *fs_stream_create(fs_stream_config_t *cfg, audio_stream_type_t type)
{
    fs_stream_t *stream = (fs_stream_t *) calloc(1, sizeof(fs_stream_t));
    configASSERT(stream);

    stream->base.cfg.task_stack_size = FATFS_STREAM_TASK_STACK_SIZE;
    stream->base.cfg.task_priority = FATFS_STREAM_TASK_PRIORITY;
    stream->base.cfg.buf_size = FATFS_STREAM_BUFFER_SIZE;

    stream->base.identifier = STREAM_TYPE_FS;

    /* Set stream specific operations */
    stream->base.cfg.derived_context_init = parse_fs_config;
    stream->base.cfg.derived_context_cleanup = reset_fs_config;
    stream->base.cfg.derived_read = fs_read;
    stream->base.cfg.derived_write = fs_write;
    stream->base.cfg.derived_on_event = NULL;

    if (type == STREAM_TYPE_WRITER)
        stream->base.cfg.w.input_wait = portMAX_DELAY;
    else
        stream->base.cfg.w.output_wait = portMAX_DELAY;

    stream->base.type = type;
    fs_stream_set_config(stream, cfg);

    return stream;
}

fs_stream_t *fs_reader_stream_create(fs_stream_config_t *cfg)
{
    return fs_stream_create(cfg, STREAM_TYPE_READER);
}

fs_stream_t *fs_writer_stream_create(fs_stream_config_t *cfg)
{
    return fs_stream_create(cfg, STREAM_TYPE_WRITER);
}

esp_err_t fs_stream_destroy(fs_stream_t *stream)
{
    fs_stream_free(stream);
    return ESP_OK;
}

esp_err_t fs_stream_set_config(fs_stream_t *stream, fs_stream_config_t *cfg)
{
    if (stream == NULL || cfg == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    snprintf(stream->cfg.file_path, FATFS_STREAM_FILE_PATH_MAX, "%s", cfg->file_path);
    return ESP_OK;
}
