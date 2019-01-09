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
#include <str_utils.h>
#include <string.h>
#include <stdio.h>
#include <va_mem_utils.h>
#include <esp_log.h>

static const char *TAG = "[str_utils]";

void blob_create_or_append(char **current_data, size_t current_size, const char *data, int size)
{
    if (*current_data == NULL) {
        *current_data = (char *) va_mem_alloc(size + 1, VA_MEM_EXTERNAL);
    } else {
        *current_data = (char *) va_mem_realloc(*current_data, current_size + size + 1, VA_MEM_EXTERNAL);
    }
    if (*current_data) {
        for (int i = 0; i < size; i++) {
            (*current_data)[current_size] = data[i];
            current_size++;
        }
        (*current_data)[current_size] = '\0';
    } else {
        ESP_LOGE(TAG, "Memory not allocated");
    }
}

int estr_append(estr_t *estr, const char *str, ...)
{
    short available_len = estr->buf_size - estr->offset - 1;
    va_list args;
    va_start(args, str);
    /* Get total length of string that'll be formed */
    short formatted_bytes = vsnprintf(NULL, 0, str, args);
    if (formatted_bytes < 0) {
        ESP_LOGE(TAG, "Error in vsnprintf");
        return -1;
    }
    if (formatted_bytes > available_len) {
        short overflow = formatted_bytes - available_len;
        /* Calculate multiple of realloc_block_size to be added */
        short additional_blocks = (overflow / estr->realloc_block_size) + 1;
        short new_size = estr->buf_size + (additional_blocks * estr->realloc_block_size);
        estr->buf = va_mem_realloc(estr->buf, new_size, VA_MEM_EXTERNAL);
        if (!estr->buf) {
            ESP_LOGE(TAG, "Failed to realloc estr");
            return -1;
        }
        estr->buf_size = new_size;
        available_len = estr->buf_size - estr->offset - 1;
    }
    formatted_bytes = vsnprintf(estr->buf + estr->offset, available_len, str, args);
    va_end(args);
    estr->offset += formatted_bytes;
    return formatted_bytes;
}

estr_t *estr_new(size_t size, size_t realloc_block_size)
{
    estr_t *estr = (estr_t *)va_mem_alloc(sizeof(estr_t), VA_MEM_EXTERNAL);
    if (!estr) {
        return NULL;
    }
    estr->buf_size = size;
    estr->buf = va_mem_alloc(estr->buf_size, VA_MEM_EXTERNAL);
    if (realloc_block_size <= 0)
        estr->realloc_block_size = DEFAULT_REALLOC_BLOCK_SIZE;
    else
        estr->realloc_block_size = realloc_block_size;
    if (!estr->buf) {
        va_mem_free(estr);
        return NULL;
    }
    return estr;
}

void estr_delete(estr_t *estr)
{
    va_mem_free(estr->buf);
    va_mem_free(estr);
}

char *estr_get_buf_ptr(estr_t *estr)
{
    return estr->buf;
}
