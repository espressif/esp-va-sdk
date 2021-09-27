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
#ifndef _COMMON_RB_H_
#define _COMMON_RB_H_

#include <stdint.h>

#define RB_FAIL ESP_FAIL
#define RB_ABORT -1
#define RB_WRITER_FINISHED -2
#define RB_READER_UNBLOCK -3

/* What is 42? Is there any other way to expose this ? */
#define RB_FETCH_ANCHOR    -42
#define RB_NO_ANCHORS      -43

typedef void *rb_handle_t;

/* These are just used for RB_TYPE_SPECIAL */
typedef struct rb_anchor {
    /* The offset at which this anchor is set. An anchor can be set at
     * any location which hasn't yet been read. It can be set at a
     * location which has already been written, but not read.
     */
    uint64_t offset;
    /* The data corresponding to this anchor */
    void *data;
} rb_anchor_t;

/* For internal use. */
typedef enum rb_type {
    RB_TYPE_BASIC,
    RB_TYPE_SPECIAL,
    RB_TYPE_ABSTRACT,
    RB_TYPE_MAX,
} rb_type_t;

#endif
