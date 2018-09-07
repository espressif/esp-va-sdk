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
/**
* \file
*   Ring Buffer library
*/
#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "ringbuf.h"
#include "esp_log.h"
#include "esp_err.h"
#include <esp_audio_mem.h>

#define RB_TAG "RINGBUF"

ringbuf_t *rb_init(const char *name, uint32_t size)
{
    ringbuf_t *r;
    unsigned char *buf;

    if (size < 2 || !name) {
        return NULL;
    }

    r = malloc(sizeof(ringbuf_t));
    assert(r);
    buf = esp_audio_mem_calloc(1, size);
    assert(buf);

    r->name = (char *) name;
    r->base = r->readptr = r->writeptr = buf;
    r->fill_cnt = 0;
    r->size = size;

    vSemaphoreCreateBinary(r->can_read);
    assert(r->can_read);
    vSemaphoreCreateBinary(r->can_write);
    assert(r->can_write);
    r->lock = xSemaphoreCreateMutex();
    assert(r->lock);

    r->abort = 0;
    r->writer_finished = 0;
    r->reader_unblock = 0;

    return r;
}

void rb_cleanup(ringbuf_t *rb)
{
    free(rb->base);
    rb->base = NULL;
    vSemaphoreDelete(rb->can_read);
    rb->can_read = NULL;
    vSemaphoreDelete(rb->can_write);
    rb->can_write = NULL;
    vSemaphoreDelete(rb->lock);
    rb->lock = NULL;
    free(rb);
}

static void _rb_reset(ringbuf_t *rb, int abort_val)
{
    if (rb == NULL) {
        return;
    }
    rb->readptr = rb->writeptr = rb->base;
    rb->fill_cnt = 0;
    rb->writer_finished = 0;
    rb->reader_unblock = 0;
    rb_abort(rb, abort_val);
}

void rb_reset(ringbuf_t *rb)
{
    return _rb_reset(rb, 0);
}

void rb_reset_and_abort(ringbuf_t *rb)
{
    return _rb_reset(rb, 1);
}

/*
 * @brief: get the number of empty bytes available in the buffer
 */
ssize_t rb_available(ringbuf_t *r)
{
    ESP_LOGD(RB_TAG, "rb leftover %d bytes", r->size - r->fill_cnt);
    return (r->size - r->fill_cnt);
}

int rb_read(ringbuf_t *r, uint8_t *buf, int buf_len, uint32_t ticks_to_wait)
{
    int read_size;
    int total_read_size = 0;

    xSemaphoreTake(r->lock, portMAX_DELAY);

    while (buf_len) {
        if (r->fill_cnt < buf_len) {
            read_size = r->fill_cnt;
        } else {
            read_size = buf_len;
        }
        if ((r->readptr + read_size) > (r->base + r->size)) {
            int rlen1 = r->base + r->size - r->readptr;
            int rlen2 = read_size - rlen1;
            if (buf) {
                memcpy(buf, r->readptr, rlen1);
                memcpy(buf + rlen1, r->base, rlen2);
            }
            r->readptr = r->base + rlen2;
        } else {
            if (buf) {
                memcpy(buf, r->readptr, read_size);
            }
            r->readptr = r->readptr + read_size;
        }

        buf_len -= read_size;
        r->fill_cnt -= read_size;
        total_read_size += read_size;
        if (buf) {
            buf += read_size;
        }

        if (buf_len == 0) {
            break;
        }

        xSemaphoreGive(r->lock);
        if (!r->writer_finished && !r->abort && !r->reader_unblock) {
            if (xSemaphoreTake(r->can_read, ticks_to_wait) != pdTRUE) {
                goto out;
            }
        }
        if (r->abort == 1) {
            total_read_size = -1;
            goto out;
        }
        if (r->writer_finished == 1) {
            goto out;
        }
        if (r->reader_unblock == 1) {
            total_read_size = -3;
            goto out;
        }

        xSemaphoreTake(r->lock, portMAX_DELAY);
    }

    xSemaphoreGive(r->lock);
out:
    if (total_read_size > 0) {
        xSemaphoreGive(r->can_write);
    }
    if (r->writer_finished == 1 && total_read_size == 0) {
        total_read_size = -2;
    }
    r->reader_unblock = 0; /* We are anyway unblocking reader */
    return total_read_size;
}

int rb_write(ringbuf_t *r, uint8_t *buf, int buf_len, uint32_t ticks_to_wait)
{
    int write_size;
    int total_write_size = 0;

    xSemaphoreTake(r->lock, portMAX_DELAY);

    while (buf_len) {
        if ((r->size - r->fill_cnt) < buf_len) {
            write_size = r->size - r->fill_cnt;
        } else {
            write_size = buf_len;
        }
        if ((r->writeptr + write_size) > (r->base + r->size)) {
            int wlen1 = r->base + r->size - r->writeptr;
            int wlen2 = write_size - wlen1;
            memcpy(r->writeptr, buf, wlen1);
            memcpy(r->base, buf + wlen1, wlen2);
            r->writeptr = r->base + wlen2;
        } else {
            memcpy(r->writeptr, buf, write_size);
            r->writeptr = r->writeptr + write_size;
        }

        buf_len -= write_size;
        r->fill_cnt += write_size;
        total_write_size += write_size;
        buf += write_size;

        if (buf_len == 0) {
            break;
        }

        xSemaphoreGive(r->lock);
        if (r->writer_finished) {
            return write_size > 0 ? write_size : -2;
        }
        if (xSemaphoreTake(r->can_write, ticks_to_wait) != pdTRUE) {
            goto out;
        }
        if (r->abort == 1) {
            goto out;
        }
        xSemaphoreTake(r->lock, portMAX_DELAY);
    }

    xSemaphoreGive(r->lock);
out:
    if (total_write_size != 0 ) {
        xSemaphoreGive(r->can_read);
    }
    return total_write_size;
}

void rb_abort(ringbuf_t *rb, int val)
{
    if (rb == NULL) {
        return;
    }
    rb->abort = val;
    xSemaphoreGive(rb->can_read);
    xSemaphoreGive(rb->can_write);
    xSemaphoreGive(rb->lock);
}

void rb_signal_writer_finished(ringbuf_t *rb)
{
    if (rb == NULL) {
        return;
    }
    rb->writer_finished = 1;
    xSemaphoreGive(rb->can_read);
}

int rb_is_writer_finished(ringbuf_t *rb)
{
    if (rb == NULL) {
        return -1;
    }
    return (rb->writer_finished);
}
void rb_wakeup_reader(ringbuf_t *rb)
{
    if (rb == NULL) {
        return;
    }
    rb->reader_unblock = 1;
    xSemaphoreGive(rb->can_read);
}
void rb_stat(ringbuf_t *rb)
{
    xSemaphoreTake(rb->lock, portMAX_DELAY);
    ESP_LOGI(RB_TAG, "filled: %d, base: %p, read_ptr: %p, write_ptr: %p, size: %d\n",
                rb->fill_cnt, rb->base, rb->readptr, rb->writeptr, rb->size);
    xSemaphoreGive(rb->lock);
}
