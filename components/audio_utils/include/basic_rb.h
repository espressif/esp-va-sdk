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
#ifndef _RING_BUF_H_
#define _RING_BUF_H_

#include <stdint.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <common_rb.h>


/**
 * @brief Create and initialize ringbuffer.
 *
 * @param[in]  rb_name Name of the ringbuffer
 * @param[in]  size size of the ringbuffer
 * @return
 *     - ringbuffer handle
 *     - NULL if failed.
 */
rb_handle_t rb_init(const char *rb_name, uint32_t size);

/**
 * @brief Cleanup and destroy ringbuffer.
 *
 * @param[in]  rb ringbuffer handle to destroy
 *
 * @note    Memory allocated will be freed with this api. This rb should not be used after this.
 */
void rb_cleanup(rb_handle_t handle);

/**
 * @brief Abort reads on ringbuffer.
 */
void rb_abort_read(rb_handle_t handle);

/**
 * @brief Abort writes on ringbuffer.
 */
void rb_abort_write(rb_handle_t handle);

/**
 * @brief Abort both read and write operations on ringbuffer.
 *
 * @note `rb_reset` should be called on this `rb` to make it usable again.
 */
void rb_abort(rb_handle_t handle);

/**
 * @brief Reset the ringbuffer.
 *
 * This will reset ringbuffer as if new.
 *
 * @param[in]  rb ringbuffer handle to reset
 */
void rb_reset(rb_handle_t handle);

/**
 * @brief Special function to reset the buffer while keeping rb_write aborted.
 *
 *        This rb needs to be reset again before being useful.
 */
void rb_reset_and_abort_write(rb_handle_t handle);

/**
 * @brief Print buffer stats.
 *
 *        This function is responsible to print rb_size, available bytes, and buffer pointer.
 */
void rb_stat(rb_handle_t handle);

/**
 * @brief Return rb filled size.
 *
 * @param[in]  rb ringbuffer handle
 */
ssize_t rb_filled(rb_handle_t handle);

/**
 * @brief Return rb available size.
 *
 * @param[in]  rb ringbuffer handle
 */
ssize_t rb_available(rb_handle_t handle);

/**
 * @brief Read from ring buffer
 *
 * @param[in]  rb Ringbuffer handle
 * @param[in]  buf Buffer to read data in
 * @param[in]  len size of data to be read
 * @param[in]  ticks_to_wait Max wait ticks if data not available
 *
 * @return
 *     - Number of bytes read
 *     - -ve value indicating error.
 *
 * @note If `buf` is send NULL, then ring buffer will simply waste `len` number of bytes by manipulating pointers internally.
 */
int rb_read(rb_handle_t handle, uint8_t *buf, int len, uint32_t ticks_to_wait);

/**
 * @brief Read from ring buffer
 *
 * @param[in]  rb Ringbuffer handle
 * @param[in]  buf Buffer to write data from
 * @param[in]  len size of data to be written
 * @param[in]  ticks_to_wait Max wait ticks if no space available in rb
 *
 * @return
 *     - Number of bytes written
 *     - -ve value indicating error.
 */
int rb_write(rb_handle_t handle, uint8_t *buf, int len, uint32_t ticks_to_wait);

/**
 * @brief Tell ringbuffer that no more writes will be done.
 *
 * @param[in]  rb ringbuffer handle
 */
void rb_signal_writer_finished(rb_handle_t handle);

/**
 * @brief Wake up from current rb_read operation.
 *
 * @param[in]  rb ringbuffer handle
 */
void rb_wakeup_reader(rb_handle_t handle);

/**
 * @brief Check if write operations are finished.
 *
 * @param[in]  rb ringbuffer handle
 */
int rb_is_writer_finished(rb_handle_t handle);

#endif
