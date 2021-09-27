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

#include <esp_log.h>

#include <abstract_rb.h>
#include <esp_audio_mem.h>

static const char *TAG = "[abstract_rb]";

typedef struct abstract_rb {
    /* Keep rb_type_t first */
    rb_type_t type;
    struct rb_func func;
    rb_handle_t rb;
} abstract_rb_t;

rb_handle_t arb_init(const char *rb_name, uint32_t size, abstract_rb_cfg_t arb_cfg)
{
    abstract_rb_t *arb = (abstract_rb_t *)esp_audio_mem_malloc(sizeof(abstract_rb_t));
    if (arb == NULL) {
        ESP_LOGE(TAG, "arb not allocated: %s", rb_name);
        return NULL;
    }

    arb->type = RB_TYPE_ABSTRACT;
    arb->func = arb_cfg.func;

    if (!arb->func.init) {
        ESP_LOGE(TAG, "rb function not defined");
        arb_deinit((rb_handle_t)arb);
        return NULL;
    }

    arb->rb = arb->func.init(rb_name, size);
    if (arb->rb == NULL) {
        ESP_LOGE(TAG, "rb not initialised: %s", rb_name);
        arb_deinit((rb_handle_t)arb);
        return NULL;
    }

    return (rb_handle_t)arb;
}

void arb_deinit(rb_handle_t handle)
{
    if (handle == NULL) {
        ESP_LOGE(TAG, "Handle is NULL");
        return;
    }
    abstract_rb_t *arb = (abstract_rb_t *)handle;
    if (arb->type != RB_TYPE_ABSTRACT) {
        ESP_LOGE(TAG, "Incorrect rb_type: %d", arb->type);
        esp_audio_mem_free(arb);
        return;
    }
    if (!arb->func.deinit) {
        ESP_LOGE(TAG, "rb function not defined");
        esp_audio_mem_free(arb);
        return;
    }

    if (arb->rb) {
        arb->func.deinit(arb->rb);
        arb->rb = NULL;
    }

    esp_audio_mem_free(arb);
}

int arb_read(rb_handle_t handle, uint8_t *buf, int len, uint32_t ticks_to_wait)
{
    if (handle == NULL) {
        ESP_LOGE(TAG, "Handle is NULL");
        return 0;
    }
    abstract_rb_t *arb = (abstract_rb_t *)handle;
    if (arb->type != RB_TYPE_ABSTRACT) {
        ESP_LOGE(TAG, "Incorrect rb_type: %d", arb->type);
        return 0;
    }
    if (!arb->func.read) {
        ESP_LOGE(TAG, "rb function not defined");
        return 0;
    }

    return arb->func.read(arb->rb, buf, len, ticks_to_wait);
}

int arb_write(rb_handle_t handle, uint8_t *buf, int len, uint32_t ticks_to_wait)
{
    if (handle == NULL) {
        ESP_LOGE(TAG, "Handle is NULL");
        return 0;
    }
    abstract_rb_t *arb = (abstract_rb_t *)handle;
    if (arb->type != RB_TYPE_ABSTRACT) {
        ESP_LOGE(TAG, "Incorrect rb_type: %d", arb->type);
        return 0;
    }
    if (!arb->func.write) {
        ESP_LOGE(TAG, "rb function not defined");
        return 0;
    }

    return arb->func.write(arb->rb, buf, len, ticks_to_wait);
}

int arb_drain(rb_handle_t handle, uint64_t drain_upto)
{
    if (handle == NULL) {
        ESP_LOGE(TAG, "Handle is NULL");
        return 0;
    }
    abstract_rb_t *arb = (abstract_rb_t *)handle;
    if (arb->type != RB_TYPE_ABSTRACT) {
        ESP_LOGE(TAG, "Incorrect rb_type: %d", arb->type);
        return 0;
    }
    if (!arb->func.drain) {
        ESP_LOGE(TAG, "rb function not defined");
        return 0;
    }

    return arb->func.drain(arb->rb, drain_upto);
}

void arb_reset(rb_handle_t handle)
{
    if (handle == NULL) {
        ESP_LOGE(TAG, "Handle is NULL");
        return;
    }
    abstract_rb_t *arb = (abstract_rb_t *)handle;
    if (arb->type != RB_TYPE_ABSTRACT) {
        ESP_LOGE(TAG, "Incorrect rb_type: %d", arb->type);
        return;
    }
    if (!arb->func.reset) {
        ESP_LOGE(TAG, "rb function not defined");
        return;
    }

    arb->func.reset(arb->rb);
}

void arb_abort(rb_handle_t handle)
{
    if (handle == NULL) {
        ESP_LOGE(TAG, "Handle is NULL");
        return;
    }
    abstract_rb_t *arb = (abstract_rb_t *)handle;
    if (arb->type != RB_TYPE_ABSTRACT) {
        ESP_LOGE(TAG, "Incorrect rb_type: %d", arb->type);
        return;
    }
    if (!arb->func.abort) {
        ESP_LOGE(TAG, "rb function not defined");
        return;
    }

    arb->func.abort(arb->rb);
}

void arb_abort_read(rb_handle_t handle)
{
    if (handle == NULL) {
        ESP_LOGE(TAG, "Handle is NULL");
        return;
    }
    abstract_rb_t *arb = (abstract_rb_t *)handle;
    if (arb->type != RB_TYPE_ABSTRACT) {
        ESP_LOGE(TAG, "Incorrect rb_type: %d", arb->type);
        return;
    }
    if (!arb->func.abort_read) {
        ESP_LOGE(TAG, "rb function not defined");
        return;
    }

    arb->func.abort_read(arb->rb);
}

void arb_abort_write(rb_handle_t handle)
{
    if (handle == NULL) {
        ESP_LOGE(TAG, "Handle is NULL");
        return;
    }
    abstract_rb_t *arb = (abstract_rb_t *)handle;
    if (arb->type != RB_TYPE_ABSTRACT) {
        ESP_LOGE(TAG, "Incorrect rb_type: %d", arb->type);
        return;
    }
    if (!arb->func.abort_write) {
        ESP_LOGE(TAG, "rb function not defined");
        return;
    }

    arb->func.abort_write(arb->rb);
}

int arb_get_filled(rb_handle_t handle)
{
    if (handle == NULL) {
        ESP_LOGE(TAG, "Handle is NULL");
        return -1;
    }
    abstract_rb_t *arb = (abstract_rb_t *)handle;
    if (arb->type != RB_TYPE_ABSTRACT) {
        ESP_LOGE(TAG, "Incorrect rb_type: %d", arb->type);
        return -1;
    }
    if (!arb->func.get_filled) {
        ESP_LOGE(TAG, "rb function not defined");
        return -1;
    }
    return arb->func.get_filled(arb->rb);
}

int arb_get_available(rb_handle_t handle)
{
    if (handle == NULL) {
        ESP_LOGE(TAG, "Handle is NULL");
        return -1;
    }
    abstract_rb_t *arb = (abstract_rb_t *)handle;
    if (arb->type != RB_TYPE_ABSTRACT) {
        ESP_LOGE(TAG, "Incorrect rb_type: %d", arb->type);
        return -1;
    }
    if (!arb->func.get_available) {
        ESP_LOGE(TAG, "rb function not defined");
        return -1;
    }

    return arb->func.get_available(arb->rb);
}

int arb_get_read_offset(rb_handle_t handle)
{
    if (handle == NULL) {
        ESP_LOGE(TAG, "Handle is NULL");
        return -1;
    }
    abstract_rb_t *arb = (abstract_rb_t *)handle;
    if (arb->type != RB_TYPE_ABSTRACT) {
        ESP_LOGE(TAG, "Incorrect rb_type: %d", arb->type);
        return -1;
    }
    if (!arb->func.get_read_offset) {
        ESP_LOGE(TAG, "rb function not defined");
        return -1;
    }

    return arb->func.get_read_offset(arb->rb);
}

int arb_get_write_offset(rb_handle_t handle)
{
    if (handle == NULL) {
        ESP_LOGE(TAG, "Handle is NULL");
        return -1;
    }
    abstract_rb_t *arb = (abstract_rb_t *)handle;
    if (arb->type != RB_TYPE_ABSTRACT) {
        ESP_LOGE(TAG, "Incorrect rb_type: %d", arb->type);
        return -1;
    }
    if (!arb->func.get_write_offset) {
        ESP_LOGE(TAG, "rb function not defined");
        return -1;
    }

    return arb->func.get_write_offset(arb->rb);
}

void arb_reset_read_offset(rb_handle_t handle)
{
    if (handle == NULL) {
        ESP_LOGE(TAG, "Handle is NULL");
        return;
    }
    abstract_rb_t *arb = (abstract_rb_t *)handle;
    if (arb->type != RB_TYPE_ABSTRACT) {
        ESP_LOGE(TAG, "Incorrect rb_type: %d", arb->type);
        return;
    }
    if (!arb->func.reset_read_offset) {
        ESP_LOGE(TAG, "rb function not defined");
        return;
    }

    arb->func.reset_read_offset(arb->rb);
}

void arb_print_stats(rb_handle_t handle)
{
    if (handle == NULL) {
        ESP_LOGE(TAG, "Handle is NULL");
        return;
    }
    abstract_rb_t *arb = (abstract_rb_t *)handle;
    if (arb->type != RB_TYPE_ABSTRACT) {
        ESP_LOGE(TAG, "Incorrect rb_type: %d", arb->type);
        return;
    }
    if (!arb->func.print_stats) {
        ESP_LOGE(TAG, "rb function not defined");
        return;
    }

    arb->func.print_stats(arb->rb);
}

void arb_wakeup_reader(rb_handle_t handle)
{
    if (handle == NULL) {
        ESP_LOGE(TAG, "Handle is NULL");
        return;
    }
    abstract_rb_t *arb = (abstract_rb_t *)handle;
    if (arb->type != RB_TYPE_ABSTRACT) {
        ESP_LOGE(TAG, "Incorrect rb_type: %d", arb->type);
        return;
    }
    if (!arb->func.wakeup_reader) {
        ESP_LOGE(TAG, "rb function not defined");
        return;
    }

    arb->func.wakeup_reader(arb->rb);
}

void arb_signal_writer_finished(rb_handle_t handle)
{
    if (handle == NULL) {
        ESP_LOGE(TAG, "Handle is NULL");
        return;
    }
    abstract_rb_t *arb = (abstract_rb_t *)handle;
    if (arb->type != RB_TYPE_ABSTRACT) {
        ESP_LOGE(TAG, "Incorrect rb_type: %d", arb->type);
        return;
    }
    if (!arb->func.signal_writer_finished) {
        ESP_LOGE(TAG, "rb function not defined");
        return;
    }

    arb->func.signal_writer_finished(arb->rb);
}

int arb_put_anchor(rb_handle_t handle, rb_anchor_t *anchor)
{
    if (handle == NULL) {
        ESP_LOGE(TAG, "Handle is NULL");
        return 0;
    }
    abstract_rb_t *arb = (abstract_rb_t *)handle;
    if (arb->type != RB_TYPE_ABSTRACT) {
        ESP_LOGE(TAG, "Incorrect rb_type: %d", arb->type);
        return 0;
    }
    if (!arb->func.put_anchor) {
        ESP_LOGE(TAG, "rb function not defined");
        return 0;
    }

    return arb->func.put_anchor(arb->rb, anchor);
}

int arb_get_anchor(rb_handle_t handle, rb_anchor_t *anchor)
{
    if (handle == NULL) {
        ESP_LOGE(TAG, "Handle is NULL");
        return 0;
    }
    abstract_rb_t *arb = (abstract_rb_t *)handle;
    if (arb->type != RB_TYPE_ABSTRACT) {
        ESP_LOGE(TAG, "Incorrect rb_type: %d", arb->type);
        return 0;
    }
    if (!arb->func.get_anchor) {
        ESP_LOGE(TAG, "rb function not defined");
        return 0;
    }

    return arb->func.get_anchor(arb->rb, anchor);
}

int arb_put_anchor_at_current(rb_handle_t handle, rb_anchor_t *anchor)
{
    if (handle == NULL) {
        ESP_LOGE(TAG, "Handle is NULL");
        return 0;
    }
    abstract_rb_t *arb = (abstract_rb_t *)handle;
    if (arb->type != RB_TYPE_ABSTRACT) {
        ESP_LOGE(TAG, "Incorrect rb_type: %d", arb->type);
        return 0;
    }
    if (!arb->func.put_anchor_at_current) {
        ESP_LOGE(TAG, "rb function not defined");
        return 0;
    }

    return arb->func.put_anchor_at_current(arb->rb, anchor);
}
