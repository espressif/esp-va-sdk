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
 * Return the Absolute of any value
 * Operation: res = abs(in0) ;
 */
static inline int32_t esp_abs(int32_t in0)
{
    int res = 0;
    asm volatile("abs %0, %1" : "=r"(res) : "r"(in0));
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
 * This is the assembly function for following c code
 * * will only work for even N value
 *  for (int i = 0; i < N; i++)
 *       {
 *         mac = esp_MAC16_16(a[i], b[i], mac);
 *       }
 *  return mac;
 */
static inline int32_t esp_mac16_16(const int16_t* a, const int16_t* b, int32_t n, int32_t mac)
{
    n = n/2;
    asm volatile ("wsr %0, acclo \n\t"
                  "mov  a3,%3 \n\t"
                  "addi a4, %1, -4 \n\t"
                  "addi a5, %2, -4 \n\t"
                  "ldinc m0, a4 \n\t"
                  "ldinc m2, a5 \n\t"
                  "loopgtz a3, .mac_loopend%= \n\t"
                  ".mac_loop%=: \n\t"
                  "mula.dd.ll m0, m2 \n\t"
                  "mula.dd.hh.ldinc m2, a5, m0, m2 \n\t"
                  "ldinc m0, a4 \n\t"
                  "addi.n a3, a3, -1 \n\t"
                  ".mac_loopend%=: \n\t"
                  "rsr %0, acclo \n\t"
                  : "+r"(mac)
                  : "r"(a), "r"(b), "r"(n)
                  : "a3", "a4", "a5");
    return mac;
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

/**
 * Multiply two 32 bit values and return most significant 32 bits after 26th bit as a result.
 * Operation: res = (int32_t) ((int64_t) in0 * in1) >> 26
 */
static inline int32_t esp_mul32_32_q26(int32_t in0, int32_t in1)
{
    int32_t res = 0, tmp = 0;
    asm volatile ("mull  %0, %2, %3 \n\t"
                  "mulsh %1, %2, %3 \n\t"
                  "ssai  26         \n\t"
                  "src   %0, %1, %0 \n\t"
                  : "+r"(res), "+r"(tmp): "r"(in0), "r"(in1));
    return res;
}

/**
 * Multiply two 32 bit values and return most significant 32 bits after 27th bit as a result.
 * Operation: res = (int32_t) ((int64_t) in0 * in1) >> 27
 */
static inline int32_t esp_mul32_32_q27(int32_t in0, int32_t in1)
{
    int32_t res = 0, tmp = 0;
    asm volatile ("mull  %0, %2, %3 \n\t"
                  "mulsh %1, %2, %3 \n\t"
                  "ssai  27         \n\t"
                  "src   %0, %1, %0 \n\t"
                  : "+r"(res), "+r"(tmp): "r"(in0), "r"(in1));
    return res;
}

/**
 * Multiply two 32 bit values and return most significant 32 bits after 28th bit as a result.
 * Operation: res = (int32_t) ((int64_t) in0 * in1) >> 28
 */
static inline int32_t esp_mul32_32_q28(int32_t in0, int32_t in1)
{
    int32_t res = 0, tmp = 0;
    asm volatile ("mull  %0, %2, %3 \n\t"
                  "mulsh %1, %2, %3 \n\t"
                  "ssai  28         \n\t"
                  "src   %0, %1, %0 \n\t"
                  : "+r"(res), "+r"(tmp): "r"(in0), "r"(in1));
    return res;
}

/**
 * Multiply two 32 bit values and return most significant 32 bits after 29th bit as a result.
 * Operation: res = (int32_t) ((int64_t) in0 * in1) >> 29
 */
static inline int32_t esp_mul32_32_q29(int32_t in0, int32_t in1)
{
    int32_t res = 0, tmp = 0;
    asm volatile ("mull  %0, %2, %3 \n\t"
                  "mulsh %1, %2, %3 \n\t"
                  "ssai  29         \n\t"
                  "src   %0, %1, %0 \n\t"
                  : "+r"(res), "+r"(tmp): "r"(in0), "r"(in1));
    return res;
}

/**
 * Multiply two 32 bit values and return most significant 32 bits after 30th bit as a result.
 * Operation: res = (int32_t) ((int64_t) in0 * in1) >> 30
 */
static inline int32_t esp_mul32_32_q30(int32_t in0, int32_t in1)
{
    int32_t res = 0, tmp = 0;
    asm volatile ("mull  %0, %2, %3 \n\t"
                  "mulsh %1, %2, %3 \n\t"
                  "ssai  30         \n\t"
                  "src   %0, %1, %0 \n\t"
                  : "+r"(res), "+r"(tmp): "r"(in0), "r"(in1));
    return res;
}

/**
 * Multiply two 32 bit values and return most significant 32 bits after 30th bit added to
 * the third argument (in2_acc)  as result.
 * Operation: res = ((int32_t) ((int64_t) in0 * in1) >> 30)+in2_acc)
 */
static inline int32_t esp_mac32_32_q30(int32_t in0, int32_t in1, int32_t in2_acc)
{
    int32_t res = 0;
    asm volatile ("mull  a3, %1, %2 \n\t"
                  "mulsh %0, %1, %2 \n\t"
                  "ssai  30         \n\t"
                  "src   a3, %0, a3 \n\t"
                  "add.n %0, a3, %3 \n\t"
                  : "+r"(res)
                  : "r"(in0), "r"(in1), "r"(in2_acc)
                  : "a3");
    return res;
}

/**
 * Multiply two 32 bit values and return most significant 32 bits added to the
 * first argument variable passed (in2_acc) as a result.
 * Operation: res = ((int32_t) ((int64_t) in0 * in1) >> 32)+in2_acc)
 */
static inline int32_t esp_mac32_32_q32(int32_t in2_acc, int32_t in0, int32_t in1)
{
    int32_t res = 0;
    asm volatile ("mulsh %0, %1, %2 \n\t"
                  "add.n %0, %0, %3 \n\t"
                  :"+r"(res)
                  : "r"(in0), "r"(in1), "r"(in2_acc));
    return res;
}

/**
 * Multiply two 32 bit values and return most significant 32 bits subtracted from the
 * first argument variable passed (in2_acc) as a result.
 * Operation: res = (in2_sub-(int32_t) ((int64_t) in0 * in1) >> 32))
 */
static inline int32_t esp_msb32_32_q32(int32_t in2_sub, int32_t in0, int32_t in1)
{
    int32_t res = 0;
    asm volatile ("mulsh %0, %1, %2 \n\t"
                  "sub   %0, %3, %0 \n\t"
                  :"+r"(res)
                  : "r"(in0), "r"(in1), "r"(in2_sub));
    return res;
}
#endif /* _COMMON_MACROS_H_ */
