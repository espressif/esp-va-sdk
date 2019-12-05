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
    http_playlist.c : File contains functions related to playlist operations.
    http_playlist_read_data : connects to url from playlist to play one by one.
    playlist_add_entry : Add an url to playlist.
    playlist_free : Free playlist.
*/

#include <esp_err.h>
#include <esp_log.h>
#include <http_playback_stream.h>
#include <http_playlist.h>
#include <esp_audio_mem.h>
#include <string.h>

#define TAG   "HTTP_PLAYLIST"

esp_err_t playlist_add_entry(http_playlist_t *playlist, char *line, const char *host_url)
{
    char *tmp_str = NULL;
    playlist_entry_t *new = (playlist_entry_t *) malloc(sizeof(playlist_entry_t));
    if (new == NULL) {
        ESP_LOGE(TAG, "Not enough memory for malloc");
        return ESP_ERR_NO_MEM;
    }
    if (strncmp(line, "http", 4)) { //This is not a full URI
        tmp_str = strdup(host_url);
        if (!tmp_str) {
            ESP_LOGE(TAG, "strdup failed. line %d", __LINE__);
            goto add_entry_err1;
        }
        if (!strncmp(line, "//", 2)) { //Schemelesss URI
            char *pos = strchr(tmp_str, ':'); //Search for first ":"
            if (!pos) { //':' not found case
                goto add_entry_err2;
            }
            pos[1] = 0;
            asprintf(&(new->uri), "%s%s", tmp_str, line);
        } else { //Relative URI
            char *pos = strrchr(tmp_str, '/'); //Search for last "/"
            if (!pos) { //'/' not found case
                goto add_entry_err2;
            }
            pos[1] = 0;
            asprintf(&(new->uri), "%s%s", tmp_str, line);
        }
        free(tmp_str);
    } else {
        new->uri = esp_audio_mem_strdup(line);
    }

    playlist_entry_t *find = NULL;
    STAILQ_FOREACH(find, &playlist->head, entries) {
        if (strcmp(find->uri, new->uri) == 0) {
            ESP_LOGW(TAG, "URI exist");
            free(new->uri);
            free(new);
            return ESP_OK;
        }
    }

    STAILQ_INSERT_TAIL(&playlist->head, new, entries);
    playlist->total_entries++;
    return ESP_OK;
add_entry_err2:
    free(tmp_str);
add_entry_err1:
    free(new);
    return ESP_FAIL;
}

esp_err_t playlist_free(http_playlist_t *playlist)
{
    if (!playlist) {
        return ESP_FAIL;
    }
    playlist_entry_t *datap, *temp;
    STAILQ_FOREACH_SAFE(datap, &playlist->head, entries, temp) {
        free(datap->uri);
        free(datap);
    }
    free(playlist);
    return ESP_OK;
}

char *playlist_get_next_entry(http_playlist_t *playlist)
{
    if (!playlist) {
        return NULL;
    }
    playlist_entry_t *temp = NULL;
    char *uri;
    temp = STAILQ_FIRST(&playlist->head);
    if (temp == NULL) {
        ESP_LOGI(TAG, "No elements in list");
        return NULL;
    }

    uri = temp->uri;

    STAILQ_REMOVE_HEAD(&playlist->head, entries);
    free(temp);

    return uri;
}

/* reads http data to buf using url from list */
int http_playlist_read_data(void *base_stream, void *buf, ssize_t len)
{
    http_playback_stream_t *bstream = (http_playback_stream_t *) base_stream;
    http_playlist_t *playlist = bstream->hls_cfg.media_playlist;
    esp_err_t ret;
    int data_read = 0;
    if (playlist != NULL) {
        while (data_read == 0) {
            char *url = playlist_get_next_entry(playlist);
            if (!url) { /* playlist is empty! */
                playlist_free(playlist);
                bstream->hls_cfg.media_playlist = NULL;
                return ESP_FAIL;
            }
            http_request_delete(bstream->handle);
            ret = http_request_new(bstream->handle, ESP_HTTP_GET, url);
            free(url); /* url freed */
            if (ret < 0) {
                goto error1;
            }
            ret = http_request_send(bstream->handle, NULL, 0);
            if (ret < 0) {
                goto error2;
            }

            data_read = http_response_recv(bstream->handle, buf, len);
            continue;
error2:
            bstream->base.event_func.func(bstream->base.event_func.arg, STREAM_EVENT_FAILED, 0);
            http_request_delete(bstream->handle);
error1:
            http_connection_delete(bstream->handle);
            bstream->handle = NULL;
            /* If new or recv fails for any URL, free and abort*/
            ESP_LOGE(TAG, "Error playing playlist");

            playlist_free(playlist); /* free playlist */
            bstream->hls_cfg.media_playlist = NULL;
            return ESP_FAIL;
        }
    }
    return data_read;
}
