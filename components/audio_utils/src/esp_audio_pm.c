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

#include <esp_audio_pm.h>

esp_err_t esp_audio_pm_lock_acquire(esp_audio_pm_lock_handle_t handle)
{
#ifdef CONFIG_PM_ENABLE
    return esp_pm_lock_acquire(handle);
#else
    return ESP_OK;
#endif
}

esp_err_t esp_audio_pm_lock_release(esp_audio_pm_lock_handle_t handle)
{
#ifdef CONFIG_PM_ENABLE
    return esp_pm_lock_release(handle);
#else
    return ESP_OK;
#endif
}

esp_err_t esp_audio_pm_lock_delete(esp_audio_pm_lock_handle_t handle)
{
#ifdef CONFIG_PM_ENABLE
    return esp_pm_lock_delete(handle);
#else
    return ESP_OK;
#endif
}

esp_err_t esp_audio_pm_lock_create(esp_pm_lock_type_t lock_type, int arg,
                                   const char* name, esp_audio_pm_lock_handle_t* out_handle)
{
#ifdef CONFIG_PM_ENABLE
    return esp_pm_lock_create(lock_type, arg, name, out_handle);
#else
    return ESP_OK;
#endif
}

#ifdef __cplusplus
}
#endif
