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
 * The file contains some useful macros which can be reused.
 * These are written keeping performance in mind.
 */

#ifndef _COMMON_MACROS_H_
#define _COMMON_MACROS_H_

#include <stdint.h>

/**
 * Use assembly
 */
#ifndef ESP32_ASM
/* Force define */
#define ESP32_ASM
#endif
/**
 * Signed saturate a 32 bit value to 16 bits keeping output in 32 bit variable.
 */
static inline int32_t esp_saturate16(int32_t in)
{
#if defined ESP32_ASM
    asm volatile("clamps %0, %0, 15" : "+a"(in));
#else
    if (in > INT16_MAX) {
        return INT16_MAX;
    }
    if (in < INT16_MIN) {
        return INT16_MIN;
    }
#endif
    return in;
}

/**
 * Signed saturate a 32 bit value to 8 bits keeping output in 32 bit variable.
 */
static inline int32_t esp_saturate8(int32_t in)
{
#if defined ESP32_ASM
    asm volatile("clamps %0, %0, 7" : "+a"(in));
#else
    if (in > INT8_MAX) {
        return INT8_MAX;
    }
    if (in < INT8_MIN) {
        return INT8_MIN;
    }
#endif
    return in;
}

/**
 * Multiply two 32 bit values and return most significant 32 bits as a result.
 * Operation: res = (int32_t) ((int64_t) in0 * in1) >> 32
 */
static inline int32_t esp_mulhi32(int32_t in0, int32_t in1)
{
    int res;
#if defined ESP32_ASM
    asm volatile("mulsh %0, %1, %2" : "=r"(res) : "r" (in0), "r" (in1));
#else
    res = (int32_t) ((int64_t) in0 * in1) >> 32;
#endif
    return res;
}


/**
 * Count leading zeros in 16 bit value.
 */
static inline int32_t esp_clz16(uint16_t in)
{
    uint32_t res = (uint32_t) in;
    asm volatile("nsau %0, %0" : "+r" (res));

    /* res is having 16 more than actual value, as we typecasted from uint16_t to int32_t */
    return res - 16;
}

/**
 * Count leading zeros in 32 bit value.
 */
static inline int32_t esp_clz32(uint32_t in)
{
    asm volatile("nsau %0, %0" : "+r" (in));
    return in;
}

/**
 * Extract number of bit from 32 bit value from given pos.
 * Inputs: in is int32_t whereas pos and bits are immediate values
 */
#define esp_extract(in, pos, bits) ({       \
    asm volatile("extui %0, %0, %1, %2"     \
                  : "+r" (in)               \
                  : "i" (pos), "i" (bits)); \
    in;                                     \
})

#endif /* _COMMON_MACROS_H_ */
