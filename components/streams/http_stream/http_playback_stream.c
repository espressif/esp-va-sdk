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
#include <http_playback_stream.h>
#include <esp_audio_mem.h>
#include <http_hls.h>

static const char *TAG = "[http_playback_stream]";

static esp_err_t parse_http_config(void *base_stream)
{
    http_playback_stream_t *hstream = (http_playback_stream_t *) base_stream;
    bool is_hls = true;
    http_hls_mime_type_t mime_type;

    if (!hstream->base.event_func.func) {
        free(hstream->cfg.url);
        hstream->cfg.url = NULL;
        return 0;
    }

    if (http_playback_stream_create_or_renew_session(hstream) != ESP_OK) {
        return ESP_FAIL;
    }

    const char *content_type = http_response_get_content_type(hstream->handle);

    mime_type = http_hls_identify_and_init_playlist(&hstream->hls_cfg, content_type, hstream->handle, hstream->cfg.url);
    if (mime_type == APPLE_URL || mime_type == MPEG_URL || mime_type == XSCPLS_URL) {
        is_hls = true;
    } else {
        /* if it's not HLS, it could be a direct audio url */
        is_hls = false;
    }

    int ret = 0;
    do {
        if (is_hls) {
            mime_type = http_hls_connect_new_variant(hstream);
            if (!hstream->handle) { /* Variant playlist exhausted but no luck. */
                return ESP_FAIL;
            }
            if (mime_type == UNKNOWN_URL || mime_type == NO_URL) { /* Cannot play/connect this variant. Try next. */
                continue;
            }
        }

        http_playback_stream_custom_data_t custom_data = {
            .content_type = mime_type,
            .offset_in_ms = hstream->cfg.offset_in_ms
        };

        ret = hstream->base.event_func.func(hstream->base.event_func.arg, STREAM_EVENT_CUSTOM_DATA, &custom_data);
        if (ret == 0) { /* Play successful. set socketopts and break. */
            struct timeval tv = {
                .tv_sec = 0,
                .tv_usec = 500 * 1000, /* 500 msec */
            };
            setsockopt(hstream->handle->tls->sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
            break;
        } else { /* Couldn't play this url. */
            ESP_LOGE(TAG, "Could not play url: %s", hstream->cfg.url);
            ESP_LOGE(TAG, "content-type: %d", mime_type);

            if (is_hls) {
                continue; /* Cannot play this variant. Try next. */
            } else {
                /* This is direct url. And we cannot play. break. */
                http_request_delete(hstream->handle);
                http_connection_delete(hstream->handle);
                hstream->handle = NULL;
                return ESP_FAIL;
            }
        }
    } while (1);

    free(hstream->cfg.url);
    hstream->cfg.url = NULL;
    return ret;
}

static void reset_http_config(void *base_stream)
{
    http_playback_stream_t *stream = (http_playback_stream_t *) base_stream;
    if (stream->handle) {
        http_request_delete(stream->handle);
        http_connection_delete(stream->handle);
        stream->handle = NULL;
    }
}

static ssize_t http_read(void *s, void *buf, ssize_t len)
{
    http_playback_stream_t *bstream = (http_playback_stream_t *) s;
    int data_read = http_response_recv(bstream->handle, buf, len);
    if (data_read == -EAGAIN) {
        printf("%s: [http_response_recv]: returning EAGAIN\n", TAG);
        return 0;
    }
    if (data_read == 0) {
        if (bstream->hls_cfg.media_playlist) {
            data_read = http_playlist_read_data(bstream, buf, len);
            if (data_read == -EAGAIN) {
                printf("%s: [http_playlist_read_data]: returning EAGAIN\n", TAG);
                return 0;
            }
        } else {
            return -1;
        }
    }
    return data_read;
}

static ssize_t http_write(void *s, void *buf, ssize_t len)
{
    /* TODO */
    ssize_t data_read = -1;
    return data_read;
}

static void http_playback_stream_free(http_playback_stream_t *stream)
{
    if (!stream) {
        return;
    }
    free(stream->cfg.url);
    free(stream);
}

static http_playback_stream_t *http_playback_stream_create(http_playback_stream_config_t *cfg, audio_stream_type_t type)
{
    http_playback_stream_t *stream = (http_playback_stream_t *) calloc(1, sizeof(http_playback_stream_t));
    configASSERT(stream);

    if (cfg) {
        memcpy(&stream->cfg, cfg, sizeof(http_playback_stream_config_t));
    }

    stream->base.cfg.task_stack_size = HTTP_PLAYBACK_STREAM_TASK_STACK_SIZE;
    stream->base.cfg.task_priority = HTTP_PLAYBACK_STREAM_TASK_PRIORITY;
    stream->base.cfg.buf_size = HTTP_PLAYBACK_STREAM_BUFFER_SIZE;

    stream->base.identifier = STREAM_TYPE_HTTP;

    /* Set stream specific operations */
    stream->base.cfg.derived_context_init = parse_http_config;
    stream->base.cfg.derived_context_cleanup = reset_http_config;
    stream->base.cfg.derived_read = http_read;
    stream->base.cfg.derived_write = http_write;
    stream->base.cfg.derived_on_event = NULL;

    if (type == STREAM_TYPE_WRITER) {
        stream->base.cfg.w.input_wait = portMAX_DELAY;
    } else if (type == STREAM_TYPE_READER) {
        stream->base.cfg.w.output_wait = portMAX_DELAY;
    }

    stream->base.type = type;

    http_playback_stream_set_config(stream, cfg);

    return stream;
}

/* Creates new connection if handle is empty.
 * or continues to create new request and handles status code.
 */
esp_err_t http_playback_stream_create_or_renew_session(http_playback_stream_t *hstream)
{
    int ret = 0;
    if (!hstream->handle) {
        esp_tls_cfg_t tls_cfg = {
            .use_global_ca_store = true,
        };
        while (1) {
            ret = http_connection_new_async(hstream->cfg.url, &tls_cfg, &hstream->handle);
            if (!hstream->base._run || ret == -1) {
                ESP_LOGE(TAG, "http_connection_new_async failed! _run = %d, ret = %d, line %d", hstream->base._run, ret, __LINE__);
                return ESP_FAIL;
            } else if (ret) {
                break;
            }
            /**
             * First attempt is half way through.
             * Retry after some time to avoid watachdog trigger in case it keeps failing
             */
            vTaskDelay(10);
        };
        http_connection_set_keepalive_and_recv_timeout(hstream->handle);
    }

    do {
        http_request_delete(hstream->handle); /* Delete old request */
        if (http_connection_new_needed(hstream->handle, hstream->cfg.url)) {
            http_connection_delete(hstream->handle); /* Delete old connection */
            hstream->handle = NULL;
            /* Create new connection */
            esp_tls_cfg_t tls_cfg = {
                .use_global_ca_store = true,
            };
            while (1) {
                ret = http_connection_new_async(hstream->cfg.url, &tls_cfg, &hstream->handle);
                if (!hstream->base._run || ret == -1) {
                    ESP_LOGE(TAG, "http_connection_new_async failed! _run = %d, ret = %d, line %d", hstream->base._run, ret, __LINE__);
                    return ESP_FAIL;
                } else if (ret) {
                    break;
                }
                /**
                 * First attempt is half way through.
                 * Retry after some time delay to avoid watachdog trigger in case it keeps failing
                 */
                vTaskDelay(10);
            };
            http_connection_set_keepalive_and_recv_timeout(hstream->handle);
        }

        if(http_request_new(hstream->handle, ESP_HTTP_GET, hstream->cfg.url) < 0) {
            http_connection_delete(hstream->handle);
            hstream->handle = NULL;
            return ESP_FAIL;
        }

        if ((http_request_send(hstream->handle, NULL, 0) < 0) ||
                (http_header_fetch(hstream->handle) < 0)) {
            http_request_delete(hstream->handle);
            http_connection_delete(hstream->handle);
            hstream->handle = NULL;
            return ESP_FAIL;
        }
        int status_code = http_response_get_code(hstream->handle);
        if (status_code == 301 || status_code == 302 || status_code == 303 ||
                status_code == 305 || status_code == 307 || status_code == 308) {
            free(hstream->cfg.url);
            hstream->cfg.url = esp_audio_mem_strdup(http_response_get_redirect_location(hstream->handle));
            ESP_LOGI(TAG, "Received status code: %d. Redirecting to: %s", status_code, hstream->cfg.url);
            continue;
        } else if (status_code != 200) {
            ESP_LOGE(TAG, "Expected 200 status code, got %d instead", status_code);
            http_request_delete(hstream->handle);
            http_connection_delete(hstream->handle);
            hstream->handle = NULL;
            return ESP_FAIL;
        }
        return ESP_OK;
    } while (1);
}

esp_err_t http_playback_stream_destroy(http_playback_stream_t *stream)
{
    http_playback_stream_free(stream);
    return ESP_OK;
}

void http_playback_stream_set_stack_size(http_playback_stream_t *stream, ssize_t stack_size)
{
    if (stream == NULL) {
        return;
    }
    stream->base.cfg.task_stack_size = stack_size;
}

http_playback_stream_t *http_playback_stream_create_reader(http_playback_stream_config_t *cfg)
{
    return http_playback_stream_create(cfg, STREAM_TYPE_READER);
}

http_playback_stream_t *http_playback_stream_create_writer(http_playback_stream_config_t *cfg)
{
    return http_playback_stream_create(cfg, STREAM_TYPE_WRITER);
}

esp_err_t http_playback_stream_set_config(http_playback_stream_t *stream, http_playback_stream_config_t *cfg)
{
    if (stream == NULL || cfg == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    audio_stream_state_t state = audio_stream_get_state(&stream->base);
    if (state != STREAM_STATE_INIT && state != STREAM_STATE_STOPPED) {
        return ESP_ERR_INVALID_STATE;
    }

    // Free up url if already exists
    if (stream->cfg.url) {
        free(stream->cfg.url);
        stream->cfg.url = NULL;
    }
    if (stream->hls_cfg.media_playlist) {
        playlist_free(stream->hls_cfg.media_playlist);
        stream->hls_cfg.media_playlist = NULL;
    }
    if (stream->hls_cfg.variant_playlist) {
        playlist_free(stream->hls_cfg.variant_playlist);
        stream->hls_cfg.variant_playlist = NULL;
    }
    stream->cfg.url = strdup(cfg->url);
    stream->cfg.offset_in_ms = cfg->offset_in_ms;
    return ESP_OK;
}
