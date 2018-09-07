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
    http_m3u.c
    File contains functions related to m3u playlist.
    http_m3u_init() : initialises m3u list
    http_m3u_read_data() : gets urls
    http_m3u_free() : cleanup
*/

#include <string.h>
#include <esp_err.h>
#include <esp_log.h>
#include <m3u8_parser.h>
#include <http_m3u.h>
#include <http_stream.h>

#define TAG   "HTTP_M3U"

/* free m3u playlist */
esp_err_t http_m3u_free(void *base_stream)
{
    http_stream_t *stream = (http_stream_t *) base_stream;
    http_stream_m3u_config_t *m3u_cfg = (http_stream_m3u_config_t *) stream->m3u_cfg;
    if (m3u_cfg) {
        for (int i = 0; i < MAX_M3U_URLS; i++) {
            if (m3u_cfg->urls[i] != NULL) {
                free(m3u_cfg->urls[i]);
                m3u_cfg->urls[i] = NULL;
            }
        }
        free(stream->m3u_cfg);
        stream->m3u_cfg = NULL;
        return ESP_OK;
    }
    return ESP_ERR_INVALID_STATE;
}

/* reads http data to buf using url from m3u list */
int http_m3u_read_data(void *base_stream, void *buf, ssize_t len)
{
    http_stream_t *bstream = (http_stream_t *) base_stream;
    http_stream_m3u_config_t *m3u_cfg = (http_stream_m3u_config_t *) bstream->m3u_cfg;
    esp_err_t ret;
    int data_read = 0;
    if (bstream->m3u_cfg != NULL) {
        if (m3u_cfg->playing_index == m3u_cfg->total) {
            free(m3u_cfg->urls[m3u_cfg->playing_index - 1]);
            m3u_cfg->urls[m3u_cfg->playing_index - 1] = NULL;
            http_request_delete(bstream->handle);
            http_connection_delete(bstream->handle);
            bstream->handle = NULL;
            return -1;
        }
        while (m3u_cfg->playing_index < m3u_cfg->total && data_read == 0) {
            free(m3u_cfg->urls[m3u_cfg->playing_index - 1]);
            m3u_cfg->urls[m3u_cfg->playing_index - 1] = NULL;

            http_request_delete(bstream->handle);
            ret = http_request_new(bstream->handle, ESP_HTTP_GET, m3u_cfg->urls[m3u_cfg->playing_index]);
            if (ret < 0) {
                goto error1;
            }
            ret = http_request_send(bstream->handle, NULL, 0);
            if (ret < 0) {
                goto error2;
            }

            data_read = http_response_recv(bstream->handle, buf, len);
            m3u_cfg->playing_index++;
            continue;
error2:
            bstream->base.event_func.func(bstream->base.event_func.arg, STREAM_EVENT_FAILED, 0);
            http_request_delete(bstream->handle);
error1:
            http_connection_delete(bstream->handle);
            bstream->handle = NULL;
            /* If new or recv fails for any URL, free and abort*/
            ESP_LOGE(TAG, "Error playing m3u playlist");
            http_m3u_free(bstream);
            return -1;
        }
    }
    return data_read;
}

/* parses the url and inits the playlist with received playlist segments */
char *http_m3u_init(void *base_stream)
{
    http_stream_t *bstream = (http_stream_t *) base_stream;

    bstream->m3u_cfg = calloc(1, sizeof(http_stream_m3u_config_t));

    if (bstream->m3u_cfg == NULL) {
        ESP_LOGE(TAG, "No memory for allocating http m3u structure");
        return NULL;
    }

    http_stream_m3u_config_t *m3u_cfg = (http_stream_m3u_config_t *) bstream->m3u_cfg;
    /* The first file is a variant stream
       i.e. it consists of all possible streams of
       the same song, but with different bit rates, etc.
       Amazon Music gives us just one link right now.
    */
    m3u8_playlist_t *variants;
    variants = m3u8_parse(bstream->handle, bstream->cfg.url, NULL);
    if (variants == NULL) {
        ESP_LOGE(TAG, "Error parsing m3u URL");
        free(bstream->m3u_cfg);
        return NULL;
    }
    char *tmpurl;
    tmpurl = m3u8_get_next_segment(variants);

    ESP_LOGI (TAG, "Variant Stream - %s\n", tmpurl);
    /* Close previous parser instance before starting new one */
    m3u8_free(variants);

    /* The URL from the variant stream will have all the smaller clips*/
    m3u8_playlist_t *playlist;
    playlist = m3u8_parse(bstream->handle, tmpurl, &bstream->cfg.offset_in_ms);
    free(tmpurl);
    if (playlist == NULL) {
        ESP_LOGE(TAG, "Error parsing m3u URL");
        free(bstream->m3u_cfg);
        bstream->m3u_cfg = NULL;
        return NULL;
    }
    m3u_cfg->total = playlist->total_entries;
    m3u_cfg->playing_index = 1;

    for (int j = 0; j < m3u_cfg->total; j++) {
        tmpurl = m3u8_get_next_segment(playlist);
        if (NULL == tmpurl) {
            break;
        }
        m3u_cfg->urls[j] = tmpurl;
        //Uncomment below line for segment url list
        //ESP_LOGI(TAG, "%d/%d %s\n", j + 1, m3u_cfg->total, m3u_cfg->urls[j]);
    }

    tmpurl = NULL;
    m3u8_free(playlist);
    return m3u_cfg->urls[0];
}
