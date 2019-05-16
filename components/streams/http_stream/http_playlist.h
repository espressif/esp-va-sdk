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

#ifndef _HTTP_PLAYLIST_H_
#define _HTTP_PLAYLIST_H_

#include <unistd.h>
#include <rom/queue.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct playlist_entry_s playlist_entry_t;

struct  playlist_entry_s {
    char *uri;
    STAILQ_ENTRY(playlist_entry_s) entries;
};

typedef struct http_playlist {
    int total_entries;
    STAILQ_HEAD(stailqhead, playlist_entry_s) head;
} http_playlist_t;

esp_err_t playlist_add_entry(http_playlist_t *playlist, char *line, const char *host_url);
esp_err_t playlist_free(http_playlist_t *playlist);
char *playlist_get_next_entry(http_playlist_t *playlist);

int http_playlist_read_data(void *base_stream, void *buf, ssize_t len);

#ifdef __cplusplus
}
#endif


#endif /* _HTTP_PLAYLIST_H_ */
