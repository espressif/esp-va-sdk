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

/*
    http_xmpeg.c
    File contains functions related to m3u playlist.
    http_xmpeg_init() : initialises m3u list
    http_xmpeg_next_url() : return next url in list
    http_xmpeg_free() : cleanup // Not implemented yet.
*/

#include <string.h>
#include <esp_err.h>
#include <esp_log.h>
#include <http_stream.h>
#include <http_xmpeg.h>
#include <esp_audio_mem.h>

#define TAG   "XMPEG_URL"

static char *get_redirect_url(httpc_conn_t *h, int *num_links)
{
    printf("Inside get_redirect_url\n");
    char *a = esp_audio_mem_malloc(MAX_REDIRECT_URL_DATA_SIZE);
    *num_links = 0;
    if (!a) {
        return NULL;
    }
    size_t total_read = 0;
    int remaining_len = MAX_REDIRECT_URL_DATA_SIZE;
    while (remaining_len > 0) {
        ssize_t data_read = http_response_recv(h, a + total_read, remaining_len);
        printf("redirect-uri: Data read: %.*s with length %d\n", data_read, a + total_read, data_read);
        if (data_read < 0) {
            esp_audio_mem_free(a);
            return NULL;
        }
        if (data_read == 0) {
            break;
        }
        total_read += data_read;
        remaining_len -= data_read;
    }
    if (! total_read) {
        return NULL;
    }

    /* For safety, we set last byte of url as NULL. This might result in a corner case wherein if 256th byte is
     * \n, we might end up overwriting it and would not consider that link in the main loop. This needs to be fixed. */
    a[MAX_REDIRECT_URL_DATA_SIZE - 1] = '\0';
    /* Now the entire data is in the buffer */
    char *end = strchr(a, '\n');
    if (!end) {
        esp_audio_mem_free(a);
        return NULL;
    }

    *end = '\0';
    remaining_len = total_read - (strlen(a) + 1);
    (*num_links)++;

    while (end && remaining_len) {
        char *tmp = end;
        end = strchr(tmp + 1, '\n');
        if (end) {
            *end = '\0';
            remaining_len -= (strlen(tmp + 1) + 1);
            (*num_links)++;
        }
    }

    return a;
}

inline char *http_xmpeg_next_url(char *url)
{
    printf("Inside get_next_redirect_url\n");
    return url + strlen(url) + 1;
}

char *http_xmpeg_init(httpc_conn_t *handle, int *num_links)
{
    return get_redirect_url(handle, num_links);
}
