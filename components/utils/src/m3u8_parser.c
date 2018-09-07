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
#include <m3u8_parser.h>
#include <httpc.h>
#include <esp_audio_mem.h>

#define M3U8 "m3u8"
#define VERSION_TAG "#EXT-X-VERSION"
#define M3U_TAG "#EXTM3U"
#define INF_TAG "#EXTINF:"
#define ENDLIST_TAG "#EXT-X-ENDLIST"
#define ALLOWCACHE_TAG "#EXT-X-ALLOW-CACHE:YES"
/* This tag lists the variant streams*/
#define VARIANT_TAG "#EXT-X-STREAM-INF"
/* This tells us the maximum duration of any segment in a given playlist*/
#define TARGETDURATION_TAG "#EXT-X-TARGETDURATION"
/* This tag tells us which is the first tag in the playlist */
#define MEDIASEQUENCE_TAG "#EXT-X-MEDIA-SEQUENCE"

esp_err_t add_media_segment(m3u8_playlist_t *playlist, char *line)
{
    m3u8_media_segment_t *new = (m3u8_media_segment_t *)malloc(sizeof(m3u8_media_segment_t));
    if (new == NULL) {
        ESP_LOGE(M3U8, "Not enough memory for malloc");
        return ESP_ERR_NO_MEM;
    }
    new->uri = esp_audio_mem_strdup(line);
    STAILQ_INSERT_TAIL(&playlist->head, new, entries);
    return ESP_OK;
}

static int validate_status_code(httpc_conn_t *h, int expected_code)
{
    if (http_response_get_code(h) != expected_code) {
        ESP_LOGE(M3U8, "Fail\n");
        ESP_LOGE(M3U8, "Expected Status Code: %d, got %d\n", expected_code, http_response_get_code(h));
        return -1;
    }
    return 0;
}

esp_err_t m3u8_free(m3u8_playlist_t *playlist)
{
    m3u8_media_segment_t *datap, *temp;
    STAILQ_FOREACH_SAFE(datap, &playlist->head, entries, temp) {
        free(datap->uri);
        free(datap);
    }
    free(playlist->uri);
    free(playlist);
    playlist = NULL;
    return ESP_OK;
}

m3u8_playlist_t  *m3u8_parse(httpc_conn_t *h, char *uri, int *offset)
{
    size_t content_len = 0;
    int offset_in_ms = 0;
    if (offset) {
        offset_in_ms = *offset;
    }
    int rec_bytes = 0, total_read = 0, remaining_bytes;
    m3u8_playlist_t *playlist = NULL;
    playlist = (m3u8_playlist_t *) esp_audio_mem_malloc(sizeof(m3u8_playlist_t));
    if (playlist == NULL) {
        ESP_LOGE(M3U8, "Not enough memory for malloc");
        return NULL;
    }
    playlist->uri = esp_audio_mem_strdup(uri);
    STAILQ_INIT(&playlist->head);
    playlist->total_entries = 0;

    /* Needs to be uncommemnted once `http_response_get_content_len`
     * starts parsing for content length after setting up the connection
     * and before receiving data
     */

    ESP_LOGI(M3U8, "Parsing %s", playlist->uri);

    http_request_delete(h);
    http_request_new(h, ESP_HTTP_GET, playlist->uri);
    http_request_send(h, NULL, 0);

    http_header_fetch(h);
    content_len = http_response_get_content_len(h);
    ESP_LOGI(M3U8, "Content len is %d", content_len);

    char *buf = (char *) esp_audio_mem_calloc(1, content_len + 1);
    remaining_bytes = content_len;
    while (remaining_bytes > 0) {
        rec_bytes = http_response_recv(h, buf + total_read, remaining_bytes);
        if (rec_bytes <= 0) {
            break;
        }
        if (validate_status_code(h, 200)) {
            esp_audio_mem_free (buf);
            return NULL;
        }
        remaining_bytes -= rec_bytes;
        total_read += rec_bytes;
    }

    char *line, *b;
    line = strtok_r(buf, "\n", &b);
    int flag = 0;
    unsigned long duration = 0;
    bool stop_skip = false;

    while (line != NULL) {
        if (!strncmp(line, INF_TAG, 7)) { //this line gives us time in sec
            flag = 1;
            duration = strtoul(line + 8, NULL, 10); //ignore digits after '.' ?
        } else if (!strncmp(line, VARIANT_TAG, 17)) { //We bluntly assume, this will never happen
            flag = 1;
        } else if (!strncmp(line, ENDLIST_TAG, 14)) {
            break;
        }
        line = strtok_r(NULL, "\n", &b);
        if (flag) {
            if (!stop_skip && offset_in_ms) {
                offset_in_ms -= 1000 * duration;
                if (offset_in_ms < 0) {
                    offset_in_ms += 1000 * duration; //restore back
                    stop_skip = true;
                    add_media_segment(playlist, line);
                    playlist->total_entries++;
                }
            } else {
                add_media_segment(playlist, line);
                playlist->total_entries++;
            }
            flag = 0;
        }
    }

    if (offset) {
        *offset = offset_in_ms;
    }

    ESP_LOGI(M3U8, "Finished parsing, contents are %d -", playlist->total_entries);
    esp_audio_mem_free (buf);
    return playlist;
}

char *m3u8_get_next_segment(m3u8_playlist_t *playlist)
{
    m3u8_media_segment_t *temp = NULL;
    char *uri;
    temp = STAILQ_FIRST(&playlist->head);
    uri = temp->uri;

    if (temp == NULL) {
        ESP_LOGI(M3U8, "No elements in list");
        return NULL;
    }
    STAILQ_REMOVE_HEAD(&playlist->head, entries);
    free(temp);

    return uri;
}
