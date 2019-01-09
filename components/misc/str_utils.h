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
#ifndef _STR_UTILS_H_
#define _STR_UTILS_H_

#include <sys/types.h>
#include <string.h>
#include <va_mem_utils.h>

#define DEFAULT_REALLOC_BLOCK_SIZE  2000

typedef struct {
    short offset;
    char *buf;
    size_t buf_size;
    size_t realloc_block_size;
} estr_t;
estr_t *estr_new(size_t size, size_t realloc_block_size);
void estr_delete(estr_t *estr);
int estr_append(estr_t *estr, const char *str, ...);
void blob_create_or_append(char **current_data, size_t current_len, const char *data, int size);
char *estr_get_buf_ptr(estr_t *estr);
static inline void str_create_or_append(char **current_data, const char *data, int size)
{
    size_t current_len = 0;
    if (*current_data) {
        current_len = strlen(*current_data);
    }
    return blob_create_or_append(current_data, current_len, data, size);
}

#endif /* _STR_UTILS_H_ */
