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

#ifndef _HTTP_STREAM_H_
#define _HTTP_STREAM_H_

#include <audio_stream.h>
#include "httpc.h"
#include <unistd.h>
#include <http_m3u.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct http_stream_config {
    char *url;
    int offset_in_ms;
} http_stream_config_t;

typedef struct http_stream_custom_data {
    char *content_type;
    int offset_in_ms;
} http_stream_custom_data_t;

typedef struct http_stream {
    audio_stream_t base;
    http_stream_config_t cfg;
    void *m3u_cfg; /* holds http_stream_m3u_config_t */
    /* Private members */
    httpc_conn_t *handle;
} http_stream_t;

http_stream_t *http_stream_create_writer(http_stream_config_t *cfg);
http_stream_t *http_stream_create_reader(http_stream_config_t *cfg);

esp_err_t http_stream_destroy(http_stream_t *stream);
esp_err_t http_stream_set_config(http_stream_t *stream, http_stream_config_t *cfg);
void http_stream_set_stack_size(http_stream_t *stream, ssize_t stack_size);


#define HTTP_STREAM_BUFFER_SIZE        (512)
#define HTTP_STREAM_TASK_STACK_SIZE    10240
#define HTTP_STREAM_TASK_PRIORITY      4

#ifdef __cplusplus
}
#endif


#endif /*_HTTP_STREAM_H_*/
