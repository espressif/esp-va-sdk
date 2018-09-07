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
#ifndef _FATFS_STREAM_H_
#define _FATFS_STREAM_H_

#include <audio_stream.h>
#include <sys/types.h>

#define FATFS_STREAM_FILE_PATH_MAX      64

typedef struct fs_stream_config {
    char file_path[FATFS_STREAM_FILE_PATH_MAX];   /* Path of the file used for read/write */
} fs_stream_config_t;

typedef struct fs_stream {
    audio_stream_t base;
    fs_stream_config_t cfg;

    /* Private members */
    FILE *_file;
    size_t total_bytes;
} fs_stream_t;

fs_stream_t *fs_reader_stream_create(fs_stream_config_t *cfg);
fs_stream_t *fs_writer_stream_create(fs_stream_config_t *cfg);

esp_err_t fs_stream_destroy(fs_stream_t *stream);

esp_err_t fs_stream_set_config(fs_stream_t *stream, fs_stream_config_t *cfg);
void fs_stream_set_stack_size(fs_stream_t *stream, ssize_t stack_size);

#define FATFS_STREAM_BUFFER_SIZE (2*1024)
#define FATFS_STREAM_TASK_STACK_SIZE    2200
#define FATFS_STREAM_TASK_PRIORITY      4
#endif /* _FATFS_STREAM_H_ */
