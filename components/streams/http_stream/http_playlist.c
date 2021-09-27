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
#include <m3u8_parser.h>

#define TAG   "HTTP_PLAYLIST"
#define MAX_PLAYLIST_KEEP_TRACKS 8

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
            size_t uri_len = strlen(tmp_str) + strlen(line) + 1;
            new->uri = esp_audio_mem_calloc(1, uri_len);
            snprintf(new->uri, uri_len, "%s%s", tmp_str, line);
        } else { //Relative URI
            char *pos = strrchr(tmp_str, '/'); //Search for last "/"
            if (!pos) { //'/' not found case
                goto add_entry_err2;
            }
            pos[1] = 0;
            size_t uri_len = strlen(tmp_str) + strlen(line) + 1;
            new->uri = esp_audio_mem_calloc(1, uri_len);
            snprintf(new->uri, uri_len, "%s%s", tmp_str, line);
        }
        free(tmp_str);
    } else {
        new->uri = esp_audio_mem_strdup(line);
    }

    playlist_entry_t *find = NULL;
    STAILQ_FOREACH(find, &playlist->head, entries) {
        if (strcmp(find->uri, new->uri) == 0) {
            ESP_LOGD(TAG, "URI exists");
            esp_audio_mem_free(new->uri);
            free(new);
            return ESP_OK;
        }
    }

    new->is_played = false;
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
        esp_audio_mem_free(datap->uri);
        free(datap);
    }
    if (playlist->host_uri) {
        free(playlist->host_uri);
        playlist->host_uri = NULL;
    }
    free(playlist);
    return ESP_OK;
}

char *playlist_get_next_entry(http_playlist_t *playlist)
{
    if (!playlist) {
        return NULL;
    }

    playlist_entry_t *entry;
    char *uri = NULL;

    /* Find not played entry. */
    STAILQ_FOREACH(entry, &playlist->head, entries) {
        if (!entry->is_played) {
            entry->is_played = true;
            uri = strdup(entry->uri);
            break;
        }
    }

    if (uri) {
        /* Remove head entry if total_entries are > MAX_PLAYLIST_KEEP_TRACKS */
        if (playlist->total_entries > MAX_PLAYLIST_KEEP_TRACKS) {
            playlist_entry_t *tmp = STAILQ_FIRST(&playlist->head);
            STAILQ_REMOVE_HEAD(&playlist->head, entries);
            esp_audio_mem_free(tmp->uri);
            free(tmp);
            playlist->total_entries--;
        }
    }

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
                while (bstream->base._run && !playlist->is_complete) { /* fetch again if playlist is not complete */
                    ESP_LOGI(TAG, "Fetching again...");
                    free(bstream->cfg.url);
                    bstream->cfg.url = playlist->host_uri;
                    playlist->host_uri = NULL;
                    http_request_delete(bstream->handle);
                    http_connection_delete(bstream->handle);
                    bstream->handle = NULL;
                    if (http_playback_stream_create_or_renew_session(bstream) == ESP_FAIL) {
                        ESP_LOGE(TAG, "Failed to create connection to %s. line %d", bstream->cfg.url, __LINE__);
                        return ESP_FAIL;
                    }
                    playlist = m3u8_parse(bstream->handle, playlist, bstream->cfg.url, NULL);
                    url = playlist_get_next_entry(playlist);
                    if (!url && bstream->base._run) {
                        /**
                         * This `small` delay is very important or else we will keep trying tirelessly.
                         * In case all URLs are duplicate, we will eat up too much CPU in vain.
                         * We have already downloaded previous playlist and this one has no new URLs yet.
                         */
                        vTaskDelay(1000 / portTICK_PERIOD_MS);
                    } else {
                        break;
                    }
                };

                if (!url) { /* still no url */
                    if (playlist) {
                        playlist_free(bstream->hls_cfg.media_playlist);
                        bstream->hls_cfg.media_playlist = NULL;
                    }
                    return ESP_FAIL;
                }
            }

            http_request_delete(bstream->handle);
            ret = http_request_new(bstream->handle, ESP_HTTP_GET, url);
            esp_audio_mem_free(bstream->cfg.url); /* free old url */
            bstream->cfg.url = url; /* keep current url in cfg */

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
