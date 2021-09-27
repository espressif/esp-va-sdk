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

#ifndef _ESP_AUDIO_MEM_H_
#define _ESP_AUDIO_MEM_H_

#include <stdbool.h>
#include <string.h>
#include <esp_heap_caps.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief   allocate memory preferably in external RAM
 *
 * @param[in]  size   block size
 *
 * @return
 *     - valid pointer on success
 *     - NULL when any errors
 */
void *esp_audio_mem_malloc(int size);

/**
 * @brief   allocate zero initialized memory preferably in external RAM
 *
 * @param[in]  n      number of blocks
 * @param[in]  size   block size
 *
 * @return
 *     - valid pointer on success
 *     - NULL when any errors
 */
void *esp_audio_mem_calloc(int n, int size);

/**
 * @brief   realloc memory preferably in external RAM
 *
 * @param[in]  old_ptr    pointer to already allocated memory
 * @param[in]  old_size   old size
 * @param[in]  new_size   new size
 *
 * @return
 *     - new pointer to reallocated buffer
 *     - NULL when any errors
 */
void *esp_audio_mem_realloc(void *old_ptr, int old_size, int new_size);

/**
 * @brief   duplicate a string preferably in external RAM
 *
 * @param[in]  str    pointer to existing string
 *
 * @return
 *     - Pointer to duplicate string. This buffer must be freed by application.
 *     - NULL when any errors
 */
char *esp_audio_mem_strdup(const char *str);

/**
 * @brief   allocate dma accesible block of memory
 *
 * @param[in]  size   block size
 *
 * @return
 *     - valid pointer on success
 *     - NULL when any errors
 */
void *esp_audio_mem_alloc_dma(int n, int size);

/**
 * @brief   Free allocated buffer
 *
 * @param[in]  ptr    pointer to buffer to be freed
 */
void esp_audio_mem_free(void *ptr);

#ifdef __cplusplus
}
#endif

#endif /* _ESP_AUDIO_MEM_H_ */
