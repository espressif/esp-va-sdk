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

#define DEFAULT_CONTENT_LENGTH (16 * 1024)

http_playlist_t *m3u8_parse(httpc_conn_t *h, const char *url, int *offset)
{
    int content_len = 0;
    int offset_in_ms = 0;
    if (!h) {
        ESP_LOGE(M3U8, "http connecction handle is NULL");
        return NULL;
    }
    if (offset) {
        offset_in_ms = *offset;
    }
    int rec_bytes = 0, total_read = 0, remaining_bytes;
    http_playlist_t *playlist = NULL;
    playlist = (http_playlist_t *) esp_audio_mem_malloc(sizeof(http_playlist_t));
    if (playlist == NULL) {
        ESP_LOGE(M3U8, "Not enough memory for malloc");
        return NULL;
    }
    STAILQ_INIT(&playlist->head);
    playlist->total_entries = 0;

    /* Needs to be uncommemnted once `http_response_get_content_len`
     * starts parsing for content length after setting up the connection
     * and before receiving data
     */

    content_len = http_response_get_content_len(h);
    ESP_LOGI(M3U8, "Content len is %d", content_len);
    content_len = content_len > 0 ? content_len : DEFAULT_CONTENT_LENGTH;

    char *buf = (char *) esp_audio_mem_calloc(1, content_len + 1);
    if (!buf) {
        ESP_LOGE(M3U8, "Not able to allocate buffer of size %d", content_len + 1);
        playlist_free(playlist);
        return NULL;
    }

    remaining_bytes = content_len;
    while (remaining_bytes > 0) {
        rec_bytes = http_response_recv(h, buf + total_read, remaining_bytes);
        if (rec_bytes <= 0) {
            break;
        }
        remaining_bytes -= rec_bytes;
        total_read += rec_bytes;
    }

    if (__builtin_expect(rec_bytes > 0 && remaining_bytes < 0, false)) {
        ESP_LOGI(M3U8, "buffer of %d bytes was not enough! Song may cut short!", DEFAULT_CONTENT_LENGTH);
    }

    int flag = 0;
    unsigned long duration = 0;
    bool stop_skip = false;
    char *line, *b;

    line = strtok_r(buf, "\n", &b);
    if (line == NULL) {
        ESP_LOGE(M3U8, "No data to process! Error in http_response_recv?");
        esp_audio_mem_free (buf);
        playlist_free(playlist);
        return NULL;
    }

    if (!strncmp(line, M3U_TAG, sizeof(M3U_TAG) - 1)) { //This is EXTM3U
        while (line != NULL) {
            if (!strncmp(line, INF_TAG, sizeof(INF_TAG) - 1)) { //this line gives us time in sec
                flag = 1;
                duration = strtoul(line + 8, NULL, 10); //ignore digits after '.' ?
            } else if (!strncmp(line, VARIANT_TAG, sizeof(VARIANT_TAG) - 1)) { //We bluntly assume, this will never happen
                flag = 1;
            } else if (!strncmp(line, ENDLIST_TAG, sizeof(ENDLIST_TAG) - 1)) {
                break;
            }
            line = strtok_r(NULL, "\n", &b);
            if (!line || (flag && !strncmp(line, "#", 1))) {
                continue;
            }
            if (flag) {
                if (!stop_skip && offset_in_ms) {
                    offset_in_ms -= 1000 * duration;
                    if (offset_in_ms < 0) {
                        offset_in_ms += 1000 * duration; //restore back
                        stop_skip = true;
                        playlist_add_entry(playlist, line, url);
                    }
                } else {
                    playlist_add_entry(playlist, line, url);
                }
                flag = 0;
            }
        }
    } else { //Not EXTM3U, has listed urls. Keep adding to url list
        while (line != NULL) {
            if (!strncmp(line, "#", 1)) {
                //This is comment in playlist! Neglect this line and look next line
            } else {
                playlist_add_entry(playlist, line, url);
            }
            line = strtok_r(NULL, "\n", &b);
        }
    }

    if (offset) {
        *offset = offset_in_ms;
    }

    ESP_LOGI(M3U8, "Finished parsing, total entries: %d", playlist->total_entries);
    esp_audio_mem_free (buf);
    return playlist;
}
