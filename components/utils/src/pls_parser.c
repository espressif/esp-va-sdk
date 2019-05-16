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

#include <string.h>
#include <esp_err.h>
#include <esp_log.h>
#include <rom/queue.h>
#include <pls_parser.h>
#include <httpc.h>
#include <esp_audio_mem.h>

#define PLS_TAG "pls"

#define PLAYLIST_TAG "[playlist]"
#define FILE_TAG "File"

/* Length of the audio */
#define LENGTH_TAG "Length"

/* Number of entries: Use for verification */
#define ENTRIES_TAG "NumberOfEntries"
#define TITLE_TAG "Title"
#define VERSION_TAG "Version"

http_playlist_t  *pls_parse(httpc_conn_t *h, const char *url)
{
    size_t content_len = 0;
    int rec_bytes = 0, total_read = 0, remaining_bytes;
    if (!h) {
        ESP_LOGE(PLS_TAG, "http connecction handle is NULL");
        return NULL;
    }
    http_playlist_t *playlist = (http_playlist_t *) esp_audio_mem_malloc(sizeof(http_playlist_t));
    if (playlist == NULL) {
        ESP_LOGE(PLS_TAG, "Not enough memory for malloc");
        return NULL;
    }
    STAILQ_INIT(&playlist->head);
    playlist->total_entries = 0;

    content_len = http_response_get_content_len(h);
    ESP_LOGI(PLS_TAG, "Content len is %d", content_len);

    char *buf = (char *) esp_audio_mem_calloc(1, content_len + 1);
    remaining_bytes = content_len;
    while (remaining_bytes > 0) {
        rec_bytes = http_response_recv(h, buf + total_read, remaining_bytes);
        if (rec_bytes <= 0) {
            break;
        }
        remaining_bytes -= rec_bytes;
        total_read += rec_bytes;
    }

    char *line, *b;
    char *delimiter = " \t\n\r\v\f\b";
    line = strtok_r(buf, delimiter, &b);
    while (line != NULL) {
        if (!strncmp(line, FILE_TAG, sizeof(FILE_TAG) - 1)) { //this line gives url
            int i = 4;
            while (line[i++] != '='); //Skip till '='
            playlist_add_entry(playlist, line + i, url);
        }
        line = strtok_r(NULL, delimiter, &b);
    }

    ESP_LOGI(PLS_TAG, "Finished parsing, total entries: %d", playlist->total_entries);
    esp_audio_mem_free (buf);
    return playlist;
}
