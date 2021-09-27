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

#include <sys/time.h>
#include <esp_sntp.h>
#include <esp_log.h>
#include <esp_audio_mem.h>
#include <va_time_utils.h>

#define REF_TIME    1546300800 /* 01-Jan-2019 00:00:00 */

static const char *TAG = "[time_utils]";
static bool init_done = false;

/**
 * Info on this format: https://en.wikipedia.org/wiki/ISO_8601
 */
char *va_time_utils_get_current_time_iso8601()
{
    time_t now;
    time(&now);
    char *buf = esp_audio_mem_calloc(1, 30);
    strftime(buf, 30, "%FT%TZ", gmtime(&now));
    return buf;
}

esp_err_t va_time_utils_get_local_time_str(char *buf, size_t buf_len)
{
    struct tm timeinfo;
    char strftime_buf[64];
    time_t now;
    time(&now);
    localtime_r(&now, &timeinfo);
    strftime(strftime_buf, sizeof(strftime_buf), "%c %z[%Z]", &timeinfo);
    size_t print_size = snprintf(buf, buf_len, "%s, DST: %s", strftime_buf, timeinfo.tm_isdst ? "Yes" : "No");
    if (print_size >= buf_len) {
        ESP_LOGE(TAG, "Buffer size %d insufficient for localtime string. Required size: %d", buf_len, print_size);
        return ESP_ERR_INVALID_ARG;
    }
    return ESP_OK;
}

static esp_err_t va_time_utils_print_current_time(void)
{
    char local_time[64];
    if (va_time_utils_get_local_time_str(local_time, sizeof(local_time)) == ESP_OK) {
        if (va_time_utils_check_time() == false) {
            ESP_LOGI(TAG, "Time not synchronised yet.");
        }
        ESP_LOGI(TAG, "The current time is: %s.", local_time);
        return ESP_OK;
    }
    return ESP_FAIL;
}

esp_err_t va_time_utils_set_timezone_posix(const char *tz_posix)
{
    setenv("TZ", tz_posix, 1);
    tzset();
    va_time_utils_print_current_time();
    return ESP_OK;
}

bool va_time_utils_check_time(void)
{
    time_t now;
    time(&now);
    if (now > REF_TIME) {
        return true;
    }
    return false;
}

#define DEFAULT_TICKS   (1000 / portTICK_PERIOD_MS) /* 2 seconds in ticks */

esp_err_t va_time_utils_time_wait_for_sync(uint32_t ticks_to_wait)
{
    if (!init_done) {
        ESP_LOGW(TAG, "Time sync not initialized using 'va_time_utils_sync_init'");
    }
    ESP_LOGI(TAG, "Waiting for time to be synchronized. This may take time.");
    uint32_t ticks_remaining = ticks_to_wait;
    uint32_t ticks = DEFAULT_TICKS;
    while (ticks_remaining > 0) {
        if (va_time_utils_check_time() == true) {
            break;
        }
        ESP_LOGD(TAG, "Time not synchronized yet. Retrying...");
        ticks = ticks_remaining < DEFAULT_TICKS ? ticks_remaining : DEFAULT_TICKS;
        ticks_remaining -= ticks;
        vTaskDelay(ticks);
    }

    /* Check if ticks_to_wait expired and time is not synchronized yet. */
    if (va_time_utils_check_time() == false) {
        ESP_LOGE(TAG, "Time not synchronized within the provided ticks: %u", ticks_to_wait);
        return ESP_FAIL;
    }

    /* Get current time */
    va_time_utils_print_current_time();
    return ESP_OK;
}

esp_err_t va_time_utils_sync_init()
{
    if (sntp_enabled()) {
        ESP_LOGI(TAG, "SNTP already initialized.");
        init_done = true;
        return ESP_OK;
    }

    ESP_LOGI(TAG, "Initializing SNTP.");
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, "0.asia.pool.ntp.org");
    sntp_setservername(1, "0.cn.pool.ntp.org");
    sntp_setservername(2, "0.us.pool.ntp.org");
    sntp_init();
    init_done = true;
    return ESP_OK;
}
