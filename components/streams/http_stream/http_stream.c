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

#define HLSTAG   "HLStream"

static void http_header_cb(const char *hdr, const char *val, void *arg)
{
    http_stream_t *http_stream = (http_stream_t *) arg;

    if (!strcasecmp(hdr, "Content-Type")) {
        if (http_stream->base.event_func.func) {
            http_stream->base.event_func.func(http_stream->base.event_func.arg, STREAM_EVENT_CUSTOM_DATA, (void*) val);
        }
    }
}

static esp_err_t parse_http_config(void *base_stream)
{
    http_stream_t *bstream = (http_stream_t *) base_stream;
    int ret;
    ESP_LOGI(HLSTAG, "Passing argument as %s to connection_new", bstream->cfg.url);

    esp_tls_cfg_t tls_cfg = {
        .use_global_ca_store = true,
    };

    while (1) {
        ret = http_connection_new_async(bstream->cfg.url, &tls_cfg, &bstream->handle);
        if (!bstream->base._run || ret == -1) {
            ESP_LOGE(HLSTAG, "http_connection_new_async failed! _run = %d, ret = %d, line %d", bstream->base._run, ret, __LINE__);
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
    http_connection_set_keepalive_and_recv_timeout(bstream->handle);

    if (bstream->base.type == STREAM_TYPE_READER) {
        ret = http_request_new(bstream->handle, ESP_HTTP_GET, bstream->cfg.url);
        if (ret == -1) {
            goto error1;
        }

        http_response_set_header_cb(bstream->handle, http_header_cb, base_stream);

        ret = http_request_send(bstream->handle, NULL, 0);
        if (ret == -1) {
            goto error;
        }
    } else if (bstream->base.type == STREAM_TYPE_WRITER) {
        ret = http_request_new(bstream->handle, ESP_HTTP_POST, bstream->cfg.url);
        if (ret == -1) {
            goto error1;
        }
    }
    return ESP_OK;
error:
    http_request_delete(bstream->handle);
error1:
    http_connection_delete(bstream->handle);
    bstream->handle = NULL;
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
    if (data_read == 0) {
        return -1;
    }
    return data_read;
}

ssize_t http_write(void *s, void *buf, ssize_t len)
{
    int ret = 0;
    /* Support only chunked data at present */
    http_stream_t *bstream = (http_stream_t *) s;
    if (len > 0) {
        ret = http_send_chunk(bstream->handle, (const char *)buf, len);
        if (ret) {
            return -1;
        }
    }
    return len;
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
   if (stream == NULL)
       return;
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

    if (cfg && cfg->reuse_conn) {
        stream->base.cfg.derived_context_cleanup = NULL;
    }

    if (cfg && cfg->prev_conn_handle) {
        stream->handle = cfg->prev_conn_handle;
        stream->base.cfg.derived_context_init = NULL;
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
    if (cfg->url) {
       if (stream->cfg.url) {
            free(stream->cfg.url);
        }
        stream->cfg.url = strdup(cfg->url);
    }

    if (cfg->reuse_conn) {
        stream->base.cfg.derived_context_cleanup = NULL;
    }

    if (cfg->prev_conn_handle) {
        stream->handle = cfg->prev_conn_handle;
        stream->base.cfg.derived_context_init = NULL;
    }

    return ESP_OK;
}
