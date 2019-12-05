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
#include <audio_pipeline.h>
#include <string.h>
#include <esp_err.h>
#include <esp_log.h>
#include <esp_system.h>

#define ap_d(...) \
        ESP_LOGI("AudioPipeline", ##__VA_ARGS__)

#define ap_e(...) \
        ESP_LOGE("AudioPipeline", ##__VA_ARGS__)

static esp_err_t audio_pipe_event_cb(void *arg, int event, void *data)
{
    audio_pipe_t *p = (audio_pipe_t *) arg;
    audio_stream_t *stream = NULL;

    if (event >= CODEC_EVENT_START_NUM) {
        ap_d("Codec event %d", event);
    } else {
        stream = (audio_stream_t *) data;
        assert(stream);

        if (stream->type == STREAM_TYPE_READER) {
            ap_d("Reader stream event %d", event);
        } else {
            ap_d("Writer stream event %d", event);
        }
    }

    lock(p->lock);
    p->old_state = p->state;
    switch (event) {
    case STREAM_EVENT_STARTED:
        p->state = AUDIO_PIPE_STARTED;
        break;
    case STREAM_EVENT_STOPPED:
        p->state = AUDIO_PIPE_STOPPED;
        break;
    case STREAM_EVENT_PAUSED:
        p->state = AUDIO_PIPE_PAUSED;
        break;
    case CODEC_EVENT_SET_FREQ: {
        unlock(p->lock);
        if (p->event_func.func) {
            p->event_func.func(p->event_func.arg, AUDIO_PIPE_CHANGE_FREQ, data);
        }
        lock(p->lock);
        break;
    }
    case CODEC_EVENT_STARTED:
        p->state = AUDIO_PIPE_STARTED;
        break;
    case CODEC_EVENT_STOPPED:
        p->state = AUDIO_PIPE_STOPPED;
        break;
    case CODEC_EVENT_PAUSED:
        p->state = AUDIO_PIPE_PAUSED;
        break;
    default:
        break;
    }
    unlock(p->lock);

    // Send to event to application only if pipeline state changes
    if ((p->old_state != p->state) && p->event_func.func) {
        p->event_func.func(p->event_func.arg, (int) p->state, data);
    }

    return ESP_OK;
}

static audio_pipe_t *audio_pipe_alloc(const char *name)
{
    if (!name) {
        return NULL;
    }

    audio_pipe_t *p = calloc(1, sizeof(audio_pipe_t));
    assert(p);

    STAILQ_INIT(&p->pb);

    p->lock = xSemaphoreCreateMutex();
    assert(p->lock);
    p->state = AUDIO_PIPE_INITED;
    p->name = name;
    return p;
}

static void _create_insert_block(audio_pipe_t *p, void *cfg, block_type_t type, ringbuf_t *rb, size_t rb_size, bool head)
{
    audio_pipe_block_t *b = calloc(1, sizeof(audio_pipe_block_t));
    assert(b);

    b->block_cfg = cfg;
    b->btype = type;
    b->rb = rb;
    b->rb_size = rb_size;

    lock(p->lock);
    if (head) {
        STAILQ_INSERT_HEAD(&p->pb, b, next);
    } else {
        STAILQ_INSERT_TAIL(&p->pb, b, next);
    }
    p->cnt++;
    unlock(p->lock);

    return;
}

static esp_err_t audio_pipe_replace_stream(audio_pipe_t *p, audio_pipe_block_t *b, audio_stream_t *new_stream)
{
    audio_stream_t *old_stream = (audio_stream_t *) b->block_cfg;
    if (old_stream == new_stream) {
        return ESP_OK;
    }

    audio_io_fn_arg_t stream_io;
    // Copy old stream configuration
    if (old_stream->type == STREAM_TYPE_READER) {
        memcpy(&stream_io, &old_stream->op.stream_output, sizeof(audio_io_fn_arg_t));
    } else {
        memcpy(&stream_io, &old_stream->op.stream_input, sizeof(audio_io_fn_arg_t));
    }

    // Initialize new stream
    if (audio_stream_init((audio_stream_t *) new_stream, "rstream", &stream_io, &old_stream->event_func) != ESP_OK) {
        ap_d("Error initializing audio stream");
        unlock(p->lock);
        return ESP_FAIL;
    }
    // Destroy old stream
    audio_stream_destroy(old_stream);

    lock(p->lock);
    b->block_cfg = new_stream;
    unlock(p->lock);

    return ESP_OK;
}

static esp_err_t audio_pipe_replace_codec(audio_pipe_t *p, audio_pipe_block_t *b, audio_codec_t *new_codec)
{
    audio_codec_t *old_codec = (audio_codec_t *) b->block_cfg;
    if (old_codec == new_codec) {
        return ESP_OK;
    }

    // Initialize new codec with old configuration
    if (audio_codec_init((audio_codec_t *) new_codec, "rcodec", &old_codec->codec_input, &old_codec->codec_output,
                         &old_codec->event_func) != ESP_OK) {
        ap_d("Error initializing audio codec");
        unlock(p->lock);
        return ESP_FAIL;
    }
    // Destroy old codec
    audio_codec_destroy(old_codec);
    lock(p->lock);
    b->block_cfg = new_codec;
    unlock(p->lock);

    return ESP_OK;
}

static int _audio_pipe_stop(audio_pipe_t *p)
{
    audio_pipe_block_t *b;

    STAILQ_FOREACH(b, &p->pb, next) {
        // Only stop first block in pipeline, stop gets propagated across pipeline
        if (b->btype == STREAM_BLOCK) {
            audio_stream_stop(b->block_cfg);
            break;
        } else if (b->btype == CODEC_BLOCK) {
            audio_codec_stop(b->block_cfg);
            break;
        }
    }
    return ESP_OK;
}

static int _audio_pipe_start(audio_pipe_t *p)
{
    audio_pipe_block_t *b;

    STAILQ_FOREACH(b, &p->pb, next) {
        if (b->rb) {
            rb_reset(b->rb);
        }
        if (b->btype == STREAM_BLOCK) {
            audio_stream_start(b->block_cfg);
        } else if (b->btype == CODEC_BLOCK) {
            audio_codec_start(b->block_cfg);
        }
    }
    return ESP_OK;
}

static int _audio_pipe_pause(audio_pipe_t *p)
{
    audio_pipe_block_t *b;

    b = STAILQ_LAST(&p->pb, audio_pipe_block, next);
    if (b) {
        audio_stream_pause(b->block_cfg);
        return ESP_OK;
    }
    return ESP_FAIL;
}

static int _audio_pipe_resume(audio_pipe_t *p)
{
    audio_pipe_block_t *b;

    b = STAILQ_LAST(&p->pb, audio_pipe_block, next);
    if (b) {
        audio_stream_resume(b->block_cfg);
        return ESP_OK;
    }
    return ESP_FAIL;
}

static esp_err_t audio_state_machine(audio_pipe_t *p, audio_pipe_state_t next_state)
{
    int ret = ESP_OK;

    switch (p->state) {
    case AUDIO_PIPE_INITED:
    case AUDIO_PIPE_STOPPED:
        if (next_state == AUDIO_PIPE_STARTED) {
            _audio_pipe_start(p);
        } else {
            ret = ESP_FAIL;
        }
        break;
    case AUDIO_PIPE_STARTED:
    case AUDIO_PIPE_RESUMED:
        if (next_state == AUDIO_PIPE_PAUSED) {
            _audio_pipe_pause(p);
        } else if (next_state == AUDIO_PIPE_STOPPED) {
            _audio_pipe_stop(p);
        } else {
            ret = ESP_FAIL;
        }
        break;
    case AUDIO_PIPE_PAUSED:
        if (next_state == AUDIO_PIPE_RESUMED) {
            _audio_pipe_resume(p);
        } else {
            ret = ESP_FAIL;
        }
        break;
    default:
        ap_d("Invalid state %d\n", p->state);
        break;
    }

    if (ret == ESP_FAIL) {
        ap_d("Failed for state transition curr:%d next:%d", p->state, next_state);
    }
    return ret;
}

esp_err_t audio_pipe_destroy(audio_pipe_t *p)
{
    if (p == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    // Wait for existing operations to finish
    lock(p->lock);
    unlock(p->lock);

    audio_pipe_block_t *b, *save;
    STAILQ_FOREACH_SAFE(b, &p->pb, next, save) {
        if (b->rb) {
            rb_cleanup(b->rb);
        }
        if (b->btype == STREAM_BLOCK) {
            audio_stream_destroy(b->block_cfg);
        } else if (b->btype == CODEC_BLOCK) {
            audio_codec_destroy(b->block_cfg);
        }
        free(b);
    }

    vSemaphoreDelete(p->lock);
    free(p);
    return ESP_OK;
}

esp_err_t audio_pipe_register_event_cb(audio_pipe_t *p, audio_event_fn_arg_t *event_func)
{
    if (!p || !event_func) {
        return ESP_ERR_INVALID_ARG;
    }

    lock(p->lock);
    memcpy(&p->event_func, event_func, sizeof(audio_event_fn_arg_t));
    unlock(p->lock);

    return ESP_OK;
}

esp_err_t audio_pipe_start(audio_pipe_t *p)
{
    lock(p->lock);
    esp_err_t ret = audio_state_machine(p, AUDIO_PIPE_STARTED);
    unlock(p->lock);
    return ret;
}

esp_err_t audio_pipe_pause(audio_pipe_t *p)
{
    lock(p->lock);
    esp_err_t ret = audio_state_machine(p, AUDIO_PIPE_PAUSED);
    unlock(p->lock);
    return ret;
}

esp_err_t audio_pipe_resume(audio_pipe_t *p)
{
    lock(p->lock);
    esp_err_t ret = audio_state_machine(p, AUDIO_PIPE_RESUMED);
    unlock(p->lock);
    return ret;
}

esp_err_t audio_pipe_stop(audio_pipe_t *p)
{
    lock(p->lock);
    esp_err_t ret = audio_state_machine(p, AUDIO_PIPE_STOPPED);
    unlock(p->lock);
    return ret;
}

static ssize_t rb_read_cb(void *h, void *data, int len, uint32_t wait)
{
    return rb_read((ringbuf_t *) h, data, len, wait);
}

static ssize_t rb_write_cb(void *h, void *data, int len, uint32_t wait)
{
    if (len <= 0) {
        rb_signal_writer_finished((ringbuf_t *) h);
        return len;
    }
    return rb_write((ringbuf_t *) h, data, len, wait);
}

audio_pipe_t *_audio_pipe_create(const char *name, audio_stream_t *istream, size_t rb1_size,
                                 audio_io_fn_arg_t *io_cb, audio_codec_t *codec, size_t rb2_size,
                                 audio_stream_t *ostream)
{
    ringbuf_t *rb1 = NULL, *rb2 = NULL;
    audio_io_fn_arg_t stream_io;
    audio_io_fn_arg_t codec_input, codec_output;

    audio_pipe_t *pipe = audio_pipe_alloc(name);
    if (pipe == NULL) {
        ap_e("failed to create audio pipe");
        return NULL;
    }

    audio_event_fn_arg_t event_func = {
        .func = audio_pipe_event_cb,
        .arg = pipe
    };

    if (istream != NULL) {
        rb1 = rb_init("rb1", rb1_size);
        if (!rb1) {
            ap_e("Error creating ring buffer");
            goto err;
        }

        // Add input stream to pipeline
        stream_io.func = rb_write_cb;
        stream_io.arg = rb1;
        if (audio_stream_init(istream, "ipstream", &stream_io, &event_func) != ESP_OK) {
            ap_d("Error initializing audio stream");
            goto err;
        }
        _create_insert_block(pipe, istream, STREAM_BLOCK, rb1, rb1_size, true);
        codec_input.func = rb_read_cb;
        codec_input.arg = rb1;
    } else {
        // Add input callback to pipeline
        codec_input.func = io_cb->func;
        codec_input.arg = io_cb->arg;
        _create_insert_block(pipe, istream, CUSTOM_BLOCK, NULL, rb1_size, true);
    }

    // Add codec to audio pipeline
    if (codec != NULL) {
        rb2 = rb_init("rb2", rb2_size);
        if (!rb2) {
            ap_e("Error creating ring buffer");
            goto err;
        }

        codec_output.func = rb_write_cb;
        codec_output.arg = rb2;
        if (audio_codec_init(codec, "codec", &codec_input, &codec_output, &event_func) != ESP_OK) {
            ap_d("Error initializing audio codec");
            goto err;
        }
        _create_insert_block(pipe, codec, CODEC_BLOCK, rb2, rb2_size, false);
        stream_io.func = rb_read_cb;
        stream_io.arg = rb2;
    } else {
        stream_io.func = rb_read_cb;
        stream_io.arg = rb1;
    }

    // Add output stream to pipeline
    if (audio_stream_init(ostream, "opstream", &stream_io, &event_func) != ESP_OK) {
        ap_d("Error initializing audio stream");
        goto err;
    }
    _create_insert_block(pipe, ostream, STREAM_BLOCK, NULL, 0, false);

    return pipe;
err:
    audio_pipe_destroy(pipe);
    return NULL;
}

audio_pipe_t *audio_pipe_create(const char *name, audio_stream_t *istream, size_t rb1_size,
                                audio_codec_t *codec, size_t rb2_size, audio_stream_t *ostream)
{
    if (name == NULL || istream == NULL || rb1_size == 0 || ostream == NULL) {
        ap_e("Invalid argument/s");
        return NULL;
    }

    return _audio_pipe_create(name, istream, rb1_size, NULL, codec, rb2_size, ostream);
}

audio_pipe_t *audio_pipe_create_with_input_cb(const char *name, audio_io_fn_arg_t *io_cb,
        audio_codec_t *codec, size_t rb2_size, audio_stream_t *ostream)
{
    if (name == NULL || io_cb == NULL || ostream == NULL) {
        ap_e("Invalid argument/s");
        return NULL;
    }

    return _audio_pipe_create(name, NULL, RINGBUF1_DEFAULT_SIZE, io_cb, codec, rb2_size, ostream);
}

static audio_pipe_block_t *get_input_block(audio_pipe_t *p)
{
    lock(p->lock);
    audio_pipe_block_t *b = STAILQ_FIRST(&p->pb);
    unlock(p->lock);
    return b;
}

static audio_pipe_block_t *get_codec_block(audio_pipe_t *p)
{
    bool found = false;
    audio_pipe_block_t *b;

    lock(p->lock);
    STAILQ_FOREACH(b, &p->pb, next) {
        if (b->btype == CODEC_BLOCK) {
            found = true;
            break;
        }
    }
    unlock(p->lock);
    return found ? b : NULL;
}

audio_stream_identifier_t audio_pipe_get_ip_stream_type(audio_pipe_t *p)
{
    if (p == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    audio_pipe_block_t *b = get_input_block(p);
    if (b != NULL) {
        return (b->btype == CUSTOM_BLOCK) ? STREAM_TYPE_CALLBACK : audio_stream_get_identifier(b->block_cfg);
    } else {
        return -1;
    }
}

audio_codec_type_t audio_pipe_get_codec_type(audio_pipe_t *p)
{
    if (p == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    audio_pipe_block_t *b = get_codec_block(p);
    if (b != NULL) {
        return audio_codec_get_identifier(b->block_cfg);
    } else {
        return -1;
    }
}

esp_err_t audio_pipe_set_input_stream(audio_pipe_t *p, audio_stream_t *new_stream)
{
    if (p == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    if (p->state != AUDIO_PIPE_INITED && p->state != AUDIO_PIPE_STOPPED) {
        ap_d("Invalid pipeline state %d", p->state);
        return ESP_ERR_INVALID_STATE;
    }

    esp_err_t ret = ESP_FAIL;
    audio_pipe_block_t *b = get_input_block(p);
    if (b && b->btype == STREAM_BLOCK) {
        ret = audio_pipe_replace_stream(p, b, new_stream);
        if (ret != ESP_OK) {
            ap_d("Failed to replace stream %d\n", ret);
            return ret;
        }
    } else {
        b->rb = rb_init("rb1", b->rb_size);
        if (!b->rb) {
            ap_e("Error creating ring buffer");
            return ESP_ERR_NO_MEM;
        }

        audio_event_fn_arg_t event_func = {
            .func = audio_pipe_event_cb,
            .arg = p
        };

        audio_io_fn_arg_t stream_io;
        // Add input stream to pipeline
        stream_io.func = rb_write_cb;
        stream_io.arg = b->rb;
        if (audio_stream_init(new_stream, "ipstream", &stream_io, &event_func) != ESP_OK) {
            ap_d("Error initializing audio stream");
            return ESP_FAIL;
        }
        b->block_cfg = new_stream;
        b->btype = STREAM_BLOCK;
        audio_io_fn_arg_t io_cb = { .func = rb_read_cb, .arg = b->rb };
        b = get_codec_block(p);
        if (b != NULL) {
            audio_codec_modify_input_cb(b->block_cfg, &io_cb);
        }
    }
    return ret;
}

esp_err_t audio_pipe_set_codec(audio_pipe_t *p, audio_codec_t *new_codec)
{
    if (p == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    if (p->state != AUDIO_PIPE_INITED && p->state != AUDIO_PIPE_STOPPED) {
        ap_d("Invalid pipeline state %d", p->state);
        return ESP_ERR_INVALID_STATE;
    }

    esp_err_t ret = ESP_FAIL;
    audio_pipe_block_t *b = get_codec_block(p);
    if (b != NULL) {
        ret = audio_pipe_replace_codec(p, b, new_codec);
        if (ret != ESP_OK) {
            ap_d("Failed to replace codec %d\n", ret);
            return ret;
        }
    }
    return ret;
}

esp_err_t audio_pipe_set_input_cb(audio_pipe_t *p, audio_io_fn_arg_t *io_cb)
{
    if (p == NULL || io_cb == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    if (p->state != AUDIO_PIPE_INITED && p->state != AUDIO_PIPE_STOPPED) {
        ap_e("stream is running, cant modify");
        return ESP_ERR_INVALID_STATE;
    }

    audio_pipe_block_t *b = get_input_block(p);
    if (b->btype == STREAM_BLOCK) {
        audio_stream_destroy(b->block_cfg);
        rb_cleanup(b->rb);
        b->block_cfg = NULL;
        b->rb = NULL;
        b->btype = CUSTOM_BLOCK;
    }

    b = get_codec_block(p);
    if (b != NULL) {
        audio_codec_modify_input_cb(b->block_cfg, io_cb);
    }
    return ESP_OK;
}
