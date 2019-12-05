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

#include <sdkconfig.h>
#include <esp_audio_mem.h>

void *esp_audio_mem_malloc(int size)
{
    void *data;
#if (CONFIG_SPIRAM_SUPPORT && (CONFIG_SPIRAM_USE_CAPS_ALLOC || CONFIG_SPIRAM_USE_MALLOC))
    data = heap_caps_malloc(size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
#else
    data = malloc(size);
#endif
    return data;
}

void *esp_audio_mem_calloc(int n, int size)
{
    void *data;
#if (CONFIG_SPIRAM_SUPPORT && (CONFIG_SPIRAM_USE_CAPS_ALLOC || CONFIG_SPIRAM_USE_MALLOC))
    data = heap_caps_calloc(n, size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
#else
    data = calloc(n, size);
#endif
    return data;
}

void *esp_audio_mem_realloc(void *old_ptr, int old_size, int new_size)
{
    if (new_size <= old_size) {
        return old_ptr;
    }
    void *new_ptr = esp_audio_mem_calloc(1, new_size);
    if (new_ptr) {
        memcpy(new_ptr, old_ptr, old_size);
        free(old_ptr);
        old_ptr = NULL;
    }
    return new_ptr;
}

void *esp_audio_mem_alloc_dma(int n, int size)
{
    void *data =  NULL;
    data = heap_caps_malloc(n * size, MALLOC_CAP_DMA | MALLOC_CAP_8BIT);
    if (data) {
        memset(data, 0, n * size);
    }
    return data;
}

char *esp_audio_mem_strdup(const char *str)
{
#if (CONFIG_SPIRAM_SUPPORT && (CONFIG_SPIRAM_USE_CAPS_ALLOC || CONFIG_SPIRAM_USE_MALLOC))
    char *copy = heap_caps_malloc(strlen(str) + 1, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT); //1 extra for the '\0' NULL character
#else
    char *copy = malloc(strlen(str) + 1);
#endif
    if (copy) {
        strcpy(copy, str);
    }
    return copy;
}

void esp_audio_mem_free(void *ptr)
{
    free(ptr);
}
