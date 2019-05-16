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
#include <stdio.h>
#include <string.h>

#include <esp_err.h>
#include <esp_log.h>
#include <esp_audio_mem.h>
#include <audio_stream.h>

#define ASTAG   "audio_stream"

static const char *audio_stream_get_event_str(audio_stream_event_t event)
{
    switch (event) {
    case STREAM_EVENT_STARTED:
	return "Started";
	break;
    case STREAM_EVENT_STOPPED:
	return "Stopped";
	break;
    case STREAM_EVENT_PAUSED:
	return "Paused";
	break;
    case STREAM_EVENT_CUSTOM_DATA:
	return "Custom-data";
	break;
    case STREAM_EVENT_DESTROYED:
	return "Destroyed";
	break;
    case STREAM_EVENT_FAILED:
	return "Failed";
	break;
    default:
	return "Invalid";
	break;
    }
    return "Invalid";
}

static inline void audio_stream_generate_event(audio_stream_t *stream, audio_stream_event_t event)
{
    ESP_LOGI(ASTAG, "Stream %s Event %s", stream->label, audio_stream_get_event_str(event));
    if (stream->cfg.derived_on_event) {
        stream->cfg.derived_on_event(stream, event);
    }
    if (stream->event_func.func) {
        stream->event_func.func(stream->event_func.arg, event, stream);
    }
    return;
}

static void audio_stream_task(void *arg)
{
    int ret;
    ssize_t r_len, w_len;
    audio_stream_t *stream = (audio_stream_t *) arg;

    ESP_LOGI(ASTAG, "Starting %s stream", stream->label);

    if (stream->state != STREAM_STATE_INIT) {
        ESP_LOGI(ASTAG, "Stream already started");
        return;
    }

    while (1) {
        stream->state = STREAM_STATE_STOPPED;
        ret = xSemaphoreTake(stream->ctrl_sem, portMAX_DELAY);
        if (ret == pdFALSE) {
            ESP_LOGE(ASTAG, "Error taking semaphore");
            break;
        }
        if (stream->_destroy) {
            break;
        }
        if (!stream->_run) {
            if (stream->cfg.derived_context_cleanup) {
                stream->cfg.derived_context_cleanup(stream);
            }
            continue;
        }
        if (!stream->_pause) {
            if (stream->cfg.derived_context_init) {
                esp_err_t err = stream->cfg.derived_context_init(stream);
                if (err != ESP_OK) {
                    audio_stream_generate_event(stream, STREAM_EVENT_FAILED);
                    continue;
                }
            }
        }
        stream->_pause = 0;
        stream->state = STREAM_STATE_RUNNING;
        audio_stream_generate_event(stream, STREAM_EVENT_STARTED);
        while (stream->_run) {
            w_len = 0;
            if (stream->type == STREAM_TYPE_WRITER) {
                r_len = stream->op.stream_input.func(stream->op.stream_input.arg, stream->buf, stream->cfg.buf_size, stream->cfg.w.input_wait);
                if (r_len > 0) {
                    // printf("%s: stream: writing %d to write function\n", ASTAG, r_len);
                    w_len = stream->cfg.derived_write((void *)stream, stream->buf, r_len);
                }
            } else { /* STREAM_TYPE_READER */
                r_len = stream->cfg.derived_read((void *)stream, stream->buf, stream->cfg.buf_size);
                if (r_len > 0) {
                    /* In some cases the reader streams may return '0' indicating no data 'currently available. But
                     * sending '0' to the output_func may make it think that and end of stream has been reached.
                     * Instead, we treat '0' as a no-op, and go through the loop again.
                     * If a value < 0 was received, we will anyway break from the loop, and send a end-of-stream (0)
                     * output-func below.
                     */
                    w_len = stream->op.stream_output.func(stream->op.stream_output.arg, stream->buf, r_len, stream->cfg.w.output_wait);
                }
            }
            if (r_len < 0 || w_len < 0) {
                if (r_len == -3) {
                    /* We got a reader wakeup from outside */
                    continue;
                }
		        ESP_LOGI(ASTAG, "r_len = %d w_len = %d, stopping stream [%s]", r_len, w_len, stream->label);
                stream->_run = 0;
            }
            if (stream->_pause || stream->_destroy) {
                break;
            }
        }

        if (stream->_destroy == 1) {
            break;
        }
        if (stream->_pause == 1) {
            audio_stream_generate_event(stream, STREAM_EVENT_PAUSED);
            stream->state = STREAM_STATE_PAUSED;
        } else {
            ESP_LOGD(ASTAG, "Stack remaining is %d bytes", uxTaskGetStackHighWaterMark(NULL));
            if (stream->_run == 0 && stream->type == STREAM_TYPE_READER) {
                stream->op.stream_output.func(stream->op.stream_output.arg, NULL, 0, stream->cfg.w.output_wait);
            }
            if (stream->cfg.derived_context_cleanup) {
                stream->cfg.derived_context_cleanup(stream);
            }
            audio_stream_generate_event(stream, STREAM_EVENT_STOPPED);
	    ESP_LOGI(ASTAG, "Generated stopped event for stream %s", stream->label);
        }
    }
    ESP_LOGI(ASTAG, "Destroying stream %s", stream->label);

    audio_stream_generate_event(stream, STREAM_EVENT_DESTROYED);
    if (stream->cfg.derived_context_cleanup) {
        stream->cfg.derived_context_cleanup(stream);
    }
    stream->thread = NULL;
    stream->state = STREAM_STATE_DESTROYED;

    vTaskDelete(NULL);
    return;
}

static void audio_stream_cleanup(audio_stream_t *stream)
{
    if (stream == NULL) {
        return;
    }

    if (stream->buf) {
        free(stream->buf);
        stream->buf = NULL;
    }

    if (stream->ctrl_sem) {
        vSemaphoreDelete(stream->ctrl_sem);
        stream->ctrl_sem = NULL;
    }

    return;
}

esp_err_t audio_stream_init(audio_stream_t *stream, const char *label, audio_io_fn_arg_t *stream_io, audio_event_fn_arg_t *event_func)
{
    int ret;

    if (stream == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    if (stream->type != STREAM_TYPE_READER && stream->type != STREAM_TYPE_WRITER) {
        return ESP_ERR_INVALID_ARG;
    }

    stream->label = label;
    memcpy(&stream->event_func, event_func, sizeof(audio_event_fn_arg_t));

    stream->ctrl_sem = xSemaphoreCreateCounting(1, 0); // Block to begin
    configASSERT(stream->ctrl_sem);

    stream->state = STREAM_STATE_INIT;
    stream->_run = 0;
    stream->_pause = 0;
    stream->_destroy = 0;

    stream->buf = calloc(1, stream->cfg.buf_size);
    if (stream->buf == NULL) {
        ESP_LOGE(ASTAG, "Failed to allocate stream buffer");
        audio_stream_cleanup(stream);
        return ESP_ERR_NO_MEM;
    }

    if (stream->type == STREAM_TYPE_READER) {
        memcpy(&stream->op.stream_output, stream_io, sizeof(audio_io_fn_arg_t));
    } else {
        memcpy(&stream->op.stream_input, stream_io, sizeof(audio_io_fn_arg_t));
    }

    if(stream->identifier == STREAM_TYPE_HTTP) {
        TaskHandle_t stream_thread;
        StackType_t *stream_task_stack = (StackType_t *)esp_audio_mem_calloc(1, stream->cfg.task_stack_size);
        if(stream_task_stack == NULL) {
            ESP_LOGE(ASTAG, "Error allocating stack for HTTP task");
            return ESP_FAIL;
        }
        StaticTask_t *stream_task_buf = (StaticTask_t *)heap_caps_calloc(1, sizeof(StaticTask_t), MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
        stream_thread = xTaskCreateStatic(audio_stream_task,
                            stream->label,
                            stream->cfg.task_stack_size,
                            stream,
                            stream->cfg.task_priority,
                            stream_task_stack,
                            stream_task_buf);
        if (stream_thread == NULL) {
            ESP_LOGE(ASTAG, "Error in creating HTTP stream task");
            return ESP_FAIL;
        }
    } else {
        ret = xTaskCreatePinnedToCore(audio_stream_task,
                            stream->label,
                            stream->cfg.task_stack_size,
                            stream,
                            stream->cfg.task_priority,
                            &stream->thread, 0);
        if (ret != pdPASS) {
            ESP_LOGE(ASTAG, "Error in creating audio stream task");
            return ESP_FAIL;
        }
    }
    return ESP_OK;
}

audio_stream_identifier_t audio_stream_get_identifier(audio_stream_t *stream)
{
    if (stream == NULL) {
        return -1;
    }
    return stream->identifier;
}

audio_stream_state_t audio_stream_get_state(audio_stream_t *stream)
{
    if (stream == NULL) {
        return -1;
    }
    return stream->state;
}

esp_err_t audio_stream_start(audio_stream_t *stream)
{
    ESP_LOGI(ASTAG, "Starting audio stream %s", stream->label);
    stream->_run = 1;
    stream->_pause = 0;
    memset(stream->buf, 0, stream->cfg.buf_size);
    xSemaphoreGive(stream->ctrl_sem);
    return ESP_OK;
}

esp_err_t audio_stream_stop(audio_stream_t *stream)
{
    ESP_LOGI(ASTAG, "Stopping audio stream %s", stream->label);
    stream->_run = 0;
    stream->_pause = 0;
    xSemaphoreGive(stream->ctrl_sem);
    return ESP_OK;
}

esp_err_t audio_stream_pause(audio_stream_t *stream)
{
    ESP_LOGI(ASTAG, "Pausing audio stream %s", stream->label);
    stream->_pause = 1;
    return ESP_OK;
}

esp_err_t audio_stream_resume(audio_stream_t *stream)
{
    ESP_LOGI(ASTAG, "Resuming audio stream %s", stream->label);
    stream->_pause = 0;
    xSemaphoreGive(stream->ctrl_sem);
    return ESP_OK;
}

esp_err_t audio_stream_destroy(audio_stream_t *stream)
{
    const char *t;
    if (stream == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    stream->_destroy = 1;
    xSemaphoreGive(stream->ctrl_sem);
    while (stream->state != STREAM_STATE_DESTROYED) {
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
    t = stream->label;

    audio_stream_cleanup(stream);
    ESP_LOGI(ASTAG, "Stream %s destroyed", t);

    return ESP_OK;
}
