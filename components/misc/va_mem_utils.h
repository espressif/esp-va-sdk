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
#pragma once

#include <stddef.h>

//#define CONFIG_VA_MEM_DEBUG

enum va_mem_region {
    /* Allocate memory from internal SRAM */
    VA_MEM_INTERNAL = 0,
    /* Allocate memory from external RAM */
    VA_MEM_EXTERNAL,
    /* Let the SDK decide where to put this. This option will be enabled when heap_caps_calloc_default() and
     * heap_caps_realloc_default() are available*/
    // DEFAULT
};

void *va_mem_alloc(size_t size, enum va_mem_region region);
void va_mem_free(void *ptr);
void *va_mem_realloc(void *ptr, size_t size, enum va_mem_region region);
void va_mem_print_stats();
char *va_mem_strdup(const char *str, enum va_mem_region region);
char *va_mem_strndup(const char *str, size_t len_given, enum va_mem_region region);
