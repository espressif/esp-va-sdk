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
#ifndef _ABSTRACT_RB_H_
#define _ABSTRACT_RB_H_

#include <stdint.h>
#include <common_rb.h>
#include <basic_rb.h>
#include <special_rb.h>

#define DEFAULT_RB_TYPE_BASIC_FUNC() {                           \
    .func.init = rb_init,                                        \
    .func.deinit = rb_cleanup,                                   \
    .func.read = rb_read,                                        \
    .func.write = rb_write,                                      \
    .func.drain = NULL,                                          \
    .func.reset = rb_reset,                                      \
    .func.abort = rb_abort,                                      \
    .func.abort_read = rb_abort_read,                            \
    .func.abort_write = rb_abort_write,                          \
    .func.get_filled = rb_filled,                                \
    .func.get_available = rb_available,                          \
    .func.get_read_offset = NULL,                                \
    .func.get_write_offset = NULL,                               \
    .func.reset_read_offset = NULL,                              \
    .func.print_stats = rb_stat,                                 \
    .func.wakeup_reader = rb_wakeup_reader,                      \
    .func.signal_writer_finished = rb_signal_writer_finished,    \
    .func.put_anchor = NULL,                                     \
    .func.get_anchor = NULL,                                     \
    .func.put_anchor_at_current = NULL,                          \
}

#define DEFAULT_RB_TYPE_SPECIAL_FUNC() {                         \
    .func.init = srb_init,                                       \
    .func.deinit = NULL,                                         \
    .func.read = srb_read,                                       \
    .func.write = srb_write,                                     \
    .func.drain = srb_drain,                                     \
    .func.reset = srb_reset,                                     \
    .func.abort = srb_abort,                                     \
    .func.abort_read = NULL,                                     \
    .func.abort_write = NULL,                                    \
    .func.get_filled = srb_get_filled,                           \
    .func.get_available = NULL,                                  \
    .func.get_read_offset = srb_get_read_offset,                 \
    .func.get_write_offset = srb_get_write_offset,               \
    .func.reset_read_offset = srb_reset_read_offset,             \
    .func.print_stats = NULL,                                    \
    .func.wakeup_reader = srb_wakeup_reader,                     \
    .func.signal_writer_finished = srb_signal_writer_finished,   \
    .func.put_anchor = srb_put_anchor,                           \
    .func.get_anchor = srb_get_anchor,                           \
    .func.put_anchor_at_current = srb_put_anchor_at_current,     \
}

struct rb_func {
    rb_handle_t (*init)(const char *rb_name, uint32_t size);
    void (*deinit)(rb_handle_t handle);
    int (*read)(rb_handle_t handle, uint8_t *buf, int len, uint32_t ticks_to_wait);
    int (*write)(rb_handle_t handle, uint8_t *buf, int len, uint32_t ticks_to_wait);
    int (*drain)(rb_handle_t handle, uint64_t drain_upto);
    void (*reset)(rb_handle_t handle);
    void (*abort)(rb_handle_t handle);
    void (*abort_read)(rb_handle_t handle);
    void (*abort_write)(rb_handle_t handle);
    int (*get_filled)(rb_handle_t handle);
    int (*get_available)(rb_handle_t handle);
    int (*get_read_offset)(rb_handle_t handle);
    int (*get_write_offset)(rb_handle_t handle);
    void (*reset_read_offset)(rb_handle_t handle);
    void (*print_stats)(rb_handle_t handle);
    void (*wakeup_reader)(rb_handle_t handle);
    void (*signal_writer_finished)(rb_handle_t handle);
    int (*put_anchor)(rb_handle_t handle, rb_anchor_t *anchor);
    int (*get_anchor)(rb_handle_t handle, rb_anchor_t *anchor);
    int (*put_anchor_at_current)(rb_handle_t handle, rb_anchor_t *anchor);
};

typedef struct abstract_rb_cfg {
    struct rb_func func;
} abstract_rb_cfg_t;

rb_handle_t arb_init(const char *rb_name, uint32_t size, abstract_rb_cfg_t arb_cfg);
void arb_deinit(rb_handle_t handle);

int arb_read(rb_handle_t handle, uint8_t *buf, int len, uint32_t ticks_to_wait);
int arb_write(rb_handle_t handle, uint8_t *buf, int len, uint32_t ticks_to_wait);

int arb_drain(rb_handle_t handle, uint64_t drain_upto);
void arb_reset(rb_handle_t handle);
void arb_abort(rb_handle_t handle);
void arb_abort_read(rb_handle_t handle);
void arb_abort_write(rb_handle_t handle);

int arb_get_filled(rb_handle_t handle);
int arb_get_available(rb_handle_t handle);
int arb_get_read_offset(rb_handle_t handle);
int arb_get_write_offset(rb_handle_t handle);
void arb_reset_read_offset(rb_handle_t handle);
void arb_print_stats(rb_handle_t handle);

void arb_wakeup_reader(rb_handle_t handle);
void arb_signal_writer_finished(rb_handle_t handle);

int arb_put_anchor(rb_handle_t handle, rb_anchor_t *anchor);
int arb_get_anchor(rb_handle_t handle, rb_anchor_t *anchor);
int arb_put_anchor_at_current(rb_handle_t handle, rb_anchor_t *anchor);

#endif /* _ABSTRACT_RB_H_ */
