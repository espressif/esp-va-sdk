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
#include <http_stream.h>
#include <http_m3u.h>
#include <http_xmpeg.h>
#include <esp_audio_mem.h>

#define HLSTAG   "HLStream"

ssize_t http_read(void *s, void *buf, ssize_t len);
ssize_t http_read_m3u(void *s, void *buf, ssize_t len);



/*

TODO:

//New structure which could handle init, data and free fuctions for various content_types
//This function should add function pointers to init, data and free?
handle = handle_content_type(content_type);

// This will call init function to which above has been initialised
handle->init();

*/


static esp_err_t parse_http_config(void *base_stream)
{
    audio_stream_t *stream = (audio_stream_t *) base_stream;
    http_stream_t *bstream = (http_stream_t *) base_stream;
    char *url = bstream->cfg.url, *base_url = NULL;
    int ret, num_play_urls = 0;
    ESP_LOGI(HLSTAG, "Passing argument as %s to connection_new", url);
    stream->cfg.derived_read = http_read;
    do {
        bstream->handle = http_connection_new(url);
        if (bstream->handle == NULL) {
            goto error;
        }

        ret = http_request_new(bstream->handle, ESP_HTTP_GET, url);
        if (ret == -1) {
            goto error1;
        }

        ret = http_request_send(bstream->handle, NULL, 0);
        if (ret == -1) {
            goto error2;
        }
        http_header_fetch(bstream->handle);
        int status_code = http_response_get_code(bstream->handle);
        if (status_code == 301 || status_code == 302 || status_code == 303 || status_code == 305 || status_code == 307 || status_code == 308) {
            printf("Got response 301/302/303/305/307/308, redirecting...\n");
            if (base_url) {
                free(base_url);
                base_url = NULL;
            }
            url = strdup(http_response_get_redirect_location(bstream->handle));
            printf("Redirected url: %s\n", url);
            http_request_delete(bstream->handle);
            http_connection_delete(bstream->handle);
            base_url = url;
            continue;
        }

#define XMPEG_URL   "audio/x-mpegurl; charset=utf-8"
#define APPLE_URL   "application/vnd.apple.mpegurl"
        if (strcmp(http_response_get_content_type(bstream->handle), XMPEG_URL) == 0) {
            ESP_LOGI(HLSTAG, "This is an x-mpegurl file. Fetching and going to the redirection...");
            /* This is a file that contains more URLs, read the data,
             * and perform GET on one of those values
             */
            url = http_xmpeg_init(bstream->handle, &num_play_urls);
            if (!url) {
                ESP_LOGE(HLSTAG, "Error fetching redirect URI !");
                goto error2;
            } else {
                base_url = url;
                ESP_LOGI(HLSTAG, "Redirect URI: %s", url);
            }
        } else if (strcmp(http_response_get_content_type(bstream->handle), APPLE_URL) == 0) {
            url = http_m3u_init(bstream);
            if (!url) {
                ESP_LOGE(HLSTAG, "Error fetching first m3u URI !");
                http_m3u_free(bstream);
                goto error2;
            } else {
                stream->cfg.derived_read = http_read_m3u;
                num_play_urls = 1;
                ESP_LOGI(HLSTAG, "Setting first URL as %s", url);
            }
        } else {
            if (bstream->base.event_func.func) {
                int ret;
                http_stream_custom_data_t custom_data = {
                    .content_type = http_response_get_content_type(bstream->handle),
                    .offset_in_ms = bstream->cfg.offset_in_ms
                };
                ret = bstream->base.event_func.func(bstream->base.event_func.arg, STREAM_EVENT_CUSTOM_DATA, &custom_data);

                if (ret == 0) {
                    /* We can play this URL, break from the loop... */
                    struct timeval tv = {
                        .tv_sec = 0,
                        .tv_usec = 500 * 1000, // 500 msec
                    };
                    ret = setsockopt(bstream->handle->tls->sockfd, SOL_SOCKET,
                                     SO_RCVTIMEO, &tv, sizeof(tv));
                    break;
                }
            }
            /* We can't play this URL, try another one, or return */
            ESP_LOGI(HLSTAG, "URI of type %s found. Device only supports MP3 codec and hence skipping this URL\n",
                     http_response_get_content_type(bstream->handle));
            if (num_play_urls) {
                url = http_xmpeg_next_url(url);
                if (!url) {
                    ESP_LOGE(HLSTAG, "No mpeg URL found. Won't play anything.");
                    goto error2;
                }
                ESP_LOGE(HLSTAG, "Next redirect URI:%s", url);
            } else {
                ESP_LOGE(HLSTAG, "No mpeg URL found. Won't play anything.");
                goto error2;
            }
        }

        num_play_urls--;
        http_request_delete(bstream->handle);
        http_connection_delete(bstream->handle);
    } while (1);

    if (base_url) {
        esp_audio_mem_free(base_url);
    }

    return ESP_OK;

error2:
    bstream->base.event_func.func(bstream->base.event_func.arg, STREAM_EVENT_FAILED, 0);
    if (base_url) {
        esp_audio_mem_free(base_url);
    }
    http_request_delete(bstream->handle);
error1:
    http_connection_delete(bstream->handle);
    bstream->handle = NULL;
error:
    return ESP_FAIL;
}

static void reset_http_config(void *base_stream)
{
    http_stream_t *stream = (http_stream_t *) base_stream;
    if (stream->handle) {
        http_request_delete(stream->handle);
        http_connection_delete(stream->handle);
        stream->handle = NULL;
    }
}

ssize_t http_read(void *s, void *buf, ssize_t len)
{
    http_stream_t *bstream = (http_stream_t *) s;
    int data_read = http_response_recv(bstream->handle, buf, len);

    if (data_read == -EAGAIN) {
        printf("http-stream: returning EAGAIN\n");
        return 0;
    }
    if (data_read == 0) {
        return -1;
    }

    return data_read;
}

ssize_t http_read_m3u(void *s, void *buf, ssize_t len)
{
    http_stream_t *bstream = (http_stream_t *) s;
    int data_read = http_response_recv(bstream->handle, buf, len);
    if (data_read == -EAGAIN) {
        printf("http-stream: returning EAGAIN\n");
        return 0;
    }
    if (data_read == 0) {
        data_read = http_m3u_read_data(bstream, buf, len);
        if (data_read == -EAGAIN) {
            printf("http-stream: returning EAGAIN\n");
            return 0;
        }
    }
    return data_read;
}

ssize_t http_write(void *s, void *buf, ssize_t len)
{
    /* TODO */
    ssize_t data_read = -1;
    return data_read;
}

static void http_stream_free(http_stream_t *stream)
{
    if (!stream) {
        return;
    }
    free(stream->cfg.url);
    free(stream);
}

esp_err_t http_stream_destroy(http_stream_t *stream)
{
    http_stream_free(stream);
    return ESP_OK;
}

void http_stream_set_stack_size(http_stream_t *stream, ssize_t stack_size)
{
    if (stream == NULL) {
        return;
    }
    stream->base.cfg.task_stack_size = stack_size;
}

static http_stream_t *http_stream_create(http_stream_config_t *cfg, audio_stream_type_t type)
{
    http_stream_t *stream = (http_stream_t *) calloc(1, sizeof(http_stream_t));
    configASSERT(stream);

    if (cfg) {
        memcpy(&stream->cfg, cfg, sizeof(http_stream_config_t));
    }

    stream->base.cfg.task_stack_size = HTTP_STREAM_TASK_STACK_SIZE;
    stream->base.cfg.task_priority = HTTP_STREAM_TASK_PRIORITY;
    stream->base.cfg.buf_size = HTTP_STREAM_BUFFER_SIZE;

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

    http_stream_set_config(stream, cfg);

    return stream;
}

http_stream_t *http_stream_create_reader(http_stream_config_t *cfg)
{
    return http_stream_create(cfg, STREAM_TYPE_READER);
}

http_stream_t *http_stream_create_writer(http_stream_config_t *cfg)
{
    return http_stream_create(cfg, STREAM_TYPE_WRITER);
}

esp_err_t http_stream_set_config(http_stream_t *stream, http_stream_config_t *cfg)
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
    }
    http_m3u_free(stream);
    stream->cfg.url = strdup(cfg->url);
    stream->cfg.offset_in_ms = cfg->offset_in_ms;
    return ESP_OK;
}
