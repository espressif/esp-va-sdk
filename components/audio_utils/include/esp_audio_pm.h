/*
 * ESPRESSIF MIT License
 *
 * Copyright (c) 2020 <ESPRESSIF SYSTEMS (SHANGHAI) PTE LTD>
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

#include <esp_pm.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef CONFIG_PM_ENABLE
typedef esp_pm_lock_handle_t esp_audio_pm_lock_handle_t;
#else
typedef void* esp_audio_pm_lock_handle_t;
#endif

/**
 * @brief Take a power management lock
 *
 * @param handle handle obtained from esp_audio_pm_lock_create function
 * @return
 *      - ESP_OK on success
 *      - ESP_ERR_INVALID_ARG if the handle is invalid
 *      - ESP_ERR_NOT_SUPPORTED if CONFIG_PM_ENABLE is not enabled in sdkconfig
 */
esp_err_t esp_audio_pm_lock_acquire(esp_audio_pm_lock_handle_t handle);
/**
 * @brief Release the lock taken using esp_audio_pm_lock_acquire.
 *
 * @param handle handle obtained from esp_audio_pm_lock_create function
 * @return
 *      - ESP_OK on success
 *      - ESP_ERR_INVALID_ARG if the handle is invalid
 *      - ESP_ERR_INVALID_STATE if lock is not acquired
 *      - ESP_ERR_NOT_SUPPORTED if CONFIG_PM_ENABLE is not enabled in sdkconfig
 */
esp_err_t esp_audio_pm_lock_release(esp_audio_pm_lock_handle_t handle);

/**
 * @brief Delete a lock created using esp_audio_pm_lock_create
 *
 * The lock must be released before calling this function.
 *
 * This function must not be called from an ISR.
 *
 * @param handle handle obtained from esp_audio_pm_lock_create function
 * @return
 *      - ESP_OK on success
 *      - ESP_ERR_INVALID_ARG if the handle argument is NULL
 *      - ESP_ERR_INVALID_STATE if the lock is still acquired
 *      - ESP_ERR_NOT_SUPPORTED if CONFIG_PM_ENABLE is not enabled in sdkconfig
 */
esp_err_t esp_audio_pm_lock_delete(esp_audio_pm_lock_handle_t handle);

/**
 * @brief Initialize a lock handle for certain power management parameter
 *
 * @param lock_type Power management constraint which the lock should control
 * @param arg argument, value depends on lock_type, see esp_audio_pm_lock_type_t
 * @param name arbitrary string identifying the lock (e.g. "wifi" or "spi").
 *             Used by the esp_pm_dump_locks function to list existing locks.
 *             May be set to NULL. If not set to NULL, must point to a string which is valid
 *             for the lifetime of the lock.
 * @param[out] out_handle  handle returned from this function. Use this handle when calling
 *                         esp_audio_pm_lock_delete, esp_audio_pm_lock_acquire, esp_audio_pm_lock_release.
 *                         Must not be NULL.
 * @return
 *      - ESP_OK on success
 *      - ESP_ERR_NO_MEM if the lock structure can not be allocated
 *      - ESP_ERR_INVALID_ARG if out_handle is NULL or type argument is not valid
 *      - ESP_ERR_NOT_SUPPORTED if CONFIG_PM_ENABLE is not enabled in sdkconfig
 */
esp_err_t esp_audio_pm_lock_create(esp_pm_lock_type_t lock_type, int arg,
                                   const char* name, esp_audio_pm_lock_handle_t* out_handle);

#ifdef __cplusplus
}
#endif
