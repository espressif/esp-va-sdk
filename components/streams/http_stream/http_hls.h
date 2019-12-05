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

#ifndef _HTTP_HLS_H_
#define _HTTP_HLS_H_

#include <httpc.h>
#include <http_playlist.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    UNKNOWN_URL = -1,
    NO_URL = 0,
    APPLE_URL,
    MPEG_URL,
    XSCPLS_URL,
    AAC_URL,
    MP3_URL,
} http_hls_mime_type_t;

typedef struct {
    http_playlist_t *variant_playlist;
    http_playlist_t *media_playlist;
    http_hls_mime_type_t mime_type;
} http_stream_hls_config_t;

int http_hls_identify_and_init_playlist(http_stream_hls_config_t *hls_cfg, const char *mime_type, httpc_conn_t *base_conn_handle, char *url);
http_hls_mime_type_t http_hls_connect_new_variant(void *hstream);

#ifdef __cplusplus
}
#endif

#endif /*_HTTP_HLS_H_*/
