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
#include <esp_audio_mem.h>
#include <esp_log.h>
#include <abstract_rb_utils.h>
#include <string.h>

int arb_utils_put_anchor(rb_handle_t rb, int offset, void *data, uint32_t datalen)
{
    rb_anchor_t anchor;
    anchor.offset = offset;
    anchor.data = esp_audio_mem_malloc(datalen);
    if (anchor.data) {
        memcpy(anchor.data, data, datalen);
        arb_put_anchor(rb, &anchor);
        return 0;
    }
    return -1;
}

int arb_utils_get_anchor(rb_handle_t rb, int *offset, void *data, uint32_t datalen)
{
    rb_anchor_t anchor;
    if (arb_get_anchor(rb, &anchor) == 0) {
        *offset = anchor.offset;
        memcpy(data, anchor.data, datalen);
        esp_audio_mem_free(anchor.data);
        return 0;
    }
    return -1;
}

int arb_utils_put_anchor_at_current(rb_handle_t rb, void *data, uint32_t datalen)
{
    rb_anchor_t anchor;
    anchor.data = esp_audio_mem_malloc(datalen);
    if (anchor.data) {
        memcpy(anchor.data, data, datalen);
        arb_put_anchor_at_current(rb, &anchor);
        return 0;
    }
    return -1;
}

