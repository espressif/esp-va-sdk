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

#include <esp_err.h>
#include <esp_log.h>
#include <http_hls.h>
#include <m3u8_parser.h>
#include <pls_parser.h>
#include <string.h>
#include <http_playback_stream.h>
#include <esp_audio_mem.h>

#define TAG   "HLS"

/* For now, we can safely assume one-to-one relationship between MIME type and playlist type
 * MIME_TYPE_XMPEG  - m3u
 * MIME_TYPE_APPLE  - m3u
 * MIME_TYPE_XSCPLS - pls
 */
#define MIME_TYPE_APP_XMPEG     "application/x-mpegurl"
#define MIME_TYPE_AUDIO_XMPEG   "audio/x-mpegurl; charset=utf-8"
#define MIME_TYPE_APPLE         "application/vnd.apple.mpegurl"
#define MIME_TYPE_XSCPLS        "audio/x-scpls"

static void set_mime_type(http_stream_hls_config_t *hls_cfg, const char *mime_type)
{
    if (!strncmp(mime_type, MIME_TYPE_AUDIO_XMPEG, strlen(MIME_TYPE_AUDIO_XMPEG)) ||
        !strncmp(mime_type, MIME_TYPE_APP_XMPEG, strlen(MIME_TYPE_APP_XMPEG))) {
        hls_cfg->mime_type = MPEG_URL;
    } else if (!strncmp(mime_type, MIME_TYPE_APPLE, strlen(MIME_TYPE_APPLE))) {
        hls_cfg->mime_type = APPLE_URL;
    } else if (!strncmp(mime_type, MIME_TYPE_XSCPLS, strlen(MIME_TYPE_XSCPLS))) {
        hls_cfg->mime_type = XSCPLS_URL;
    } else {
        /* Not hls. Could be direct media url. */
        hls_cfg->mime_type = DIRECT_URL; /* not hls type */
    }
}

int http_hls_identify_and_init_playlist(http_stream_hls_config_t *hls_cfg, const char *mime_type, httpc_conn_t *base_conn_handle, char *url)
{
    set_mime_type(hls_cfg, mime_type);
    if (hls_cfg->variant_playlist) {
        playlist_free(hls_cfg->variant_playlist);
        hls_cfg->variant_playlist = NULL;
    }
    if (hls_cfg->media_playlist) {
        playlist_free(hls_cfg->media_playlist);
        hls_cfg->media_playlist = NULL;
    }
    switch (hls_cfg->mime_type) {
    case APPLE_URL:
    case MPEG_URL:
        hls_cfg->variant_playlist = m3u8_parse(base_conn_handle, url, NULL);
        break;
    case XSCPLS_URL:
        hls_cfg->variant_playlist = pls_parse(base_conn_handle, url);
        break;
    default:
        ESP_LOGI(TAG, "Not a playlist. Could be direct URL");
        return ESP_FAIL;
    }

    if (hls_cfg->variant_playlist == NULL) {
        ESP_LOGE(TAG, "Error parsing playlist");
        return ESP_FAIL;
    }
    return ESP_OK;
}

const char *http_hls_connect_new_variant(void *stream)
{
    http_playback_stream_t *hstream = (http_playback_stream_t *) stream;
    http_stream_hls_config_t *hls_cfg = &hstream->hls_cfg;

    /* Release old list from previous variant. */
    if (hls_cfg->media_playlist) {
        playlist_free(hls_cfg->media_playlist);
        hls_cfg->media_playlist = NULL;
    }

    char *url = playlist_get_next_entry(hls_cfg->variant_playlist);
    /* Free and return if no url in list. */
    if (!url) { /* Playlist is empty */
        playlist_free(hls_cfg->variant_playlist);
        hls_cfg->variant_playlist = NULL;
        http_request_delete(hstream->handle);
        http_connection_delete(hstream->handle);
        hstream->handle = NULL;
        return NULL;
    }

    /* Delete existing connection and create new one with new url. */
    free(hstream->cfg.url);
    hstream->cfg.url = url;

    if (http_playback_stream_create_or_renew_session(hstream) != ESP_OK) {
        return NULL;
    }

    char *content_type = http_response_get_content_type(hstream->handle);
    set_mime_type(hls_cfg, content_type);
    http_hls_mime_type_t type = hls_cfg->mime_type;

    switch (type) {
    case APPLE_URL:
    case MPEG_URL:
        hls_cfg->media_playlist = m3u8_parse(hstream->handle, hstream->cfg.url, &hstream->cfg.offset_in_ms);
        break;
    case XSCPLS_URL:
        hls_cfg->media_playlist = pls_parse(hstream->handle, hstream->cfg.url);
        break;
    default:
        ESP_LOGI(TAG, "Previous playlist was not variant! Move variant to media!");
        hls_cfg->media_playlist = hls_cfg->variant_playlist;
        hls_cfg->variant_playlist = NULL;
        return content_type;
    }

    if (!hls_cfg->media_playlist) {
        return NULL;
    } else {
        ESP_LOGI(TAG, "Resolved variant Stream - %s", url);
    }

    url = playlist_get_next_entry(hls_cfg->media_playlist);
    if (!url) { /* Playlist is empty */
        free(hls_cfg->media_playlist);
        hls_cfg->media_playlist = NULL;
        return NULL;
    }

    /* Delete existing connection and create new one with new url. */
    free(hstream->cfg.url);
    hstream->cfg.url = url;

    if (http_playback_stream_create_or_renew_session(hstream) != ESP_OK) {
        return NULL;
    }

    return http_response_get_content_type(hstream->handle);
}
