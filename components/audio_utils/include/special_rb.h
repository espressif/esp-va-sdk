#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#include <common_rb.h>

/* Special Ring Buffer: This is a ring-buffer that maintains offset
 * tracking of the ring buffer. We can register different points of
 * interest, called anchors within this ring buffer. These anchors are
 * can be fetched by the reader whenever it stumbles about it.
 */

/* Initialise the srb */
rb_handle_t srb_init(const char *rb_name, uint32_t size);
/* Write to an srb */
int srb_write(rb_handle_t handle, uint8_t *buf, int len, uint32_t ticks_to_wait);
/* Read from an srb. If you are at an anchor, an error
 * RB_FETCH_ANCHOR will be returned. You are supposed to then use the
 * srb_get_anchor() to read the anchor first, before proceeding to
 * read further
 */
int srb_read(rb_handle_t handle, uint8_t *buf, int len, uint32_t ticks_to_wait);
/* Put an anchor in the data stream at a particular offset */
int srb_put_anchor(rb_handle_t handle, rb_anchor_t *anchor);
/* Read the current anchor */
int srb_get_anchor(rb_handle_t handle, rb_anchor_t *anchor);
/* This puts the anchor at the current 'write' location. The value of
 * anchor->offset is ignored and overwritten with wherever the marker
 * was actually placed.
 */
int srb_put_anchor_at_current(rb_handle_t handle, rb_anchor_t *anchor);
int srb_drain(rb_handle_t handle, uint64_t drain_upto);
int srb_get_read_offset(rb_handle_t handle);
int srb_get_write_offset(rb_handle_t handle);
int srb_get_filled(rb_handle_t handle);
void srb_wakeup_reader(rb_handle_t handle);
void srb_signal_writer_finished(rb_handle_t handle);
void srb_reset(rb_handle_t handle);
void srb_abort(rb_handle_t handle);
void srb_reset_read_offset(rb_handle_t handle);
