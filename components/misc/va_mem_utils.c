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
#include "va_mem_utils.h"

#include <esp_log.h>
#include <esp_heap_caps.h>

#include <stdio.h>

static const char *TAG = "[va_mem_utils]";

enum va_mem_alloc_op {
    MALLOC,
    REALLOC,
};

static void *va_mem_alloc_op(void *ptr, size_t size, enum va_mem_region region, enum va_mem_alloc_op op)
{
    void *nptr = NULL;
    switch (region) {
    case VA_MEM_INTERNAL:
        switch (op) {
        case MALLOC:
            nptr = heap_caps_calloc(1, size, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
            break;
        case REALLOC:
            nptr = heap_caps_realloc(ptr, size, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
            break;
        default:
            break;
        }
        break;
    case VA_MEM_EXTERNAL:
        switch (op) {
        case MALLOC:
            nptr = heap_caps_calloc(1, size, MALLOC_CAP_SPIRAM);
            break;
        case REALLOC:
            nptr = heap_caps_realloc(ptr, size, MALLOC_CAP_SPIRAM);
            break;
        default:
            break;
        }
        break;
    default:
        /*
        case DEFAULT:
        switch (op) {
        case MALLOC:
            nptr = heap_caps_calloc_default(1, size);
            break;
        case REALLOC:
            nptr = heap_caps_realloc_default(ptr, size);
            break;
        default:
            break;
        }
        */
        break;
    }

#ifdef CONFIG_VA_MEM_DEBUG
    if (nptr) {
        printf("%s: %p: %s %d bytes from RAM %d\n", TAG, nptr, op == REALLOC ? "Reallocated" : "Allocated", size, region);
    } else {
        printf("%s: Failed to %s %d bytes from RAM %d\n", TAG, op == REALLOC ? "reallocate" : "allocate", size, region);
    }
    va_mem_print_stats(TAG);
#endif /* CONFIG_VA_MEM_DEBUG */
    return nptr;
}

void va_mem_print_stats(const char *event)
{
    printf("%s: INTERNAL-> Available: %d, Largest free block: %d\n", event, heap_caps_get_free_size(MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL), heap_caps_get_largest_free_block(MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL));
    printf("%s: EXTERNAL-> Available: %d, Largest free block: %d\n", event, heap_caps_get_free_size(MALLOC_CAP_SPIRAM), heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM));
}

void va_mem_free(void *ptr)
{
    heap_caps_free(ptr);
#ifdef CONFIG_VA_MEM_DEBUG
    printf("%s: %p: Freed memory\n", TAG, ptr);
    va_mem_print_stats(TAG);
#endif /* CONFIG_VA_MEM_DEBUG */
}

void *va_mem_alloc(size_t size, enum va_mem_region region)
{
    return va_mem_alloc_op(NULL, size, region, MALLOC);
}

void *va_mem_realloc(void *ptr, size_t size, enum va_mem_region region)
{
    return va_mem_alloc_op(ptr, size, region, REALLOC);
}
