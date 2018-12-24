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
/**
These funtions are a replacement for strdup and strndup
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <va_mem_utils.h>
#include <esp_log.h>

static const char *TAG = "[strdup]";

char *va_mem_strdup(const char *str, enum va_mem_region region)
{
    char *copy = (char *)va_mem_alloc(strlen(str) + 1, region);       //1 extra for the '\0' NULL character
    if (copy) {
        strcpy(copy, str);
    } else {
        ESP_LOGE(TAG, "Memory not allocated");
    }
    return copy;
}

char *va_mem_strndup(const char *str, size_t len_given, enum va_mem_region region)
{
    size_t len;
    len_given++;                                  //1 extra for the '\0' NULL character
    len = strlen(str) + 1;
    if (len < len_given) {                        //if the provided string length is less than the given copy length
        len_given = len;
    }
    char *copy = (char *)va_mem_alloc(len_given, region);
    if (copy) {
        strncpy(copy, str, len_given - 1);
        copy[len_given - 1] = '\0';
    } else {
        ESP_LOGE(TAG, "Memory not allocated");
    }
    return copy;
}
