// Copyright 2018 Espressif Systems (Shanghai) PTE LTD
// All rights reserved.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <esp_log.h>

#include <esp_audio_mem.h>
#include <basic_rb.h>
#include <va_dsp.h>
#include <common_dsp.h>

#define ENABLE_ESP_WWE                      /* Comment this to disable ESP_WWE */
#ifdef ENABLE_ESP_WWE
#include <esp_wwe.h>
#endif

#define DEFAULT_WWE_TASK_STACK (8 * 1024)
#define DEFAULT_RB_SIZE (8 * 1024)

#define PREROLL_LEN (32000 * 0.5)
#define WAKE_WORD_LEN (32000 * 0.6)
#define PREROLL_RB_SIZE (PREROLL_LEN + WAKE_WORD_LEN)   /* 32000 * (no.of sec) == (32000 is the number of bytes in 1 second of data) * (500ms of Preroll + 600ms of Alexa wakeword) */

static const char *TAG = "[common_dsp]";

enum preroll_status {
    PREROLL_IDLE,
    PREROLL_PENDING,
    PREROLL_SENT,
};

static struct dsp_data {
    int rb_size;
    int task_stack_size;
    bool detect_wakeword;
    bool ww_detected;
    bool mic_mute_enabled;
    enum preroll_status preroll_status;
    rb_handle_t mic_data;
    rb_handle_t preroll_data;
    QueueHandle_t va_queue;
    TaskHandle_t ww_detection_task_handle;
} dd;

int common_dsp_stream_audio(uint8_t *buffer, int size, int wait);

static void common_dsp_wake_word_detected()
{
    ESP_LOGI(TAG, "Sending event for wake-word command");
    struct dsp_event_data new_event = {
        .event = WW
    };
    xQueueSend(dd.va_queue, &new_event, portMAX_DELAY);
}

#ifdef ENABLE_ESP_WWE
static void send_to_preroll(uint8_t *data, int data_len)
{
    int len = rb_available(dd.preroll_data);
    if (len < data_len) {
        rb_read(dd.preroll_data, NULL, (data_len - len), portMAX_DELAY);
    }
    rb_write(dd.preroll_data, data, data_len, portMAX_DELAY);
}

static void ww_detection_task(void *arg)
{
    int frequency = esp_wwe_get_sample_rate();
    int audio_chunksize = esp_wwe_get_sample_chunksize();

    int buffer_size = (audio_chunksize * sizeof(int16_t));
    int16_t *buffer = esp_audio_mem_malloc(buffer_size);
    assert(buffer);
    int chunks=0;
    int priv_ms = 0;
    while(1) {
        if (dd.detect_wakeword) {
            common_dsp_stream_audio((uint8_t *)buffer, buffer_size, portMAX_DELAY);
            send_to_preroll((uint8_t *)buffer, buffer_size);
            dd.ww_detected = esp_wwe_detect(buffer);
            if (dd.ww_detected && dd.detect_wakeword) {
                dd.ww_detected = false;
                dd.preroll_status = PREROLL_PENDING;
                int new_ms = (chunks*audio_chunksize*1000)/frequency;
                printf("%.2f: Neural network detection triggered output %d.\n", (float)new_ms/1000.0, dd.ww_detected);
                int x = (new_ms - priv_ms);
                priv_ms = new_ms;
                if(x != 20) {
                    common_dsp_wake_word_detected();
                }
            }
            vTaskDelay(10/portTICK_RATE_MS);
            chunks++;
        } else {
            memset(buffer, 0, buffer_size);
            vTaskDelay(100/portTICK_RATE_MS);
        }
    }
}
#endif

int common_dsp_write_mic_data(void *data, int len, uint32_t wait)
{
    if (dd.mic_mute_enabled) {
        // Drop the data.
        // vTaskDelay(200/portTICK_RATE_MS);
        return 0;
    }
    return rb_write(dd.mic_data, (uint8_t *)data, len, wait);
}

int common_dsp_stream_audio(uint8_t *buffer, int size, int wait)
{
    int read_len = 0;
    if (dd.detect_wakeword == false) {
        /* Data is being sent to application. */
        /* Send pre-roll data first */
#ifdef ENABLE_ESP_WWE
        if (dd.preroll_status == PREROLL_PENDING) {
            read_len = rb_read(dd.preroll_data, buffer, size, 0);
            if (read_len <= 0) {
                dd.preroll_status = PREROLL_SENT;
            }
        }
#endif
        if (dd.preroll_status == PREROLL_IDLE || dd.preroll_status == PREROLL_SENT) {
            read_len = rb_read(dd.mic_data, buffer, size, wait);
        }
    } else {
        /* Data is being sent to WWE */
        dd.preroll_status = PREROLL_IDLE;
        read_len = rb_read(dd.mic_data, buffer, size, wait);
    }
    return read_len;
}

int common_dsp_get_ww_len()
{
    /* TODO: Get the actual WW length from the WWE and then return that from here */
    return WAKE_WORD_LEN;
}

void common_dsp_stop_capture()
{
    dd.detect_wakeword = true;
}

void common_dsp_start_capture()
{
    dd.detect_wakeword = false;
}

void common_dsp_mic_mute()
{
    dd.mic_mute_enabled = true;
    dd.detect_wakeword = false;
}

void common_dsp_mic_unmute()
{
    dd.mic_mute_enabled = false;
    dd.detect_wakeword = true;
}

void common_dsp_configure(common_dsp_config_t *cfg)
{
    if (!cfg) {
        ESP_LOGE(TAG, "config cannot be NULL");
        return;
    }
    if (cfg->ring_buffer_size > 0) {
        dd.rb_size = cfg->ring_buffer_size;
    }
    if (cfg->task_stack_size > 0) {
        dd.task_stack_size = cfg->task_stack_size;
    }
}

void common_dsp_init(QueueHandle_t queue)
{
    if (dd.rb_size <= 0) {
        dd.rb_size = DEFAULT_RB_SIZE;
    }
    if (dd.task_stack_size <= 0) {
        dd.task_stack_size = DEFAULT_WWE_TASK_STACK;
    }

    dd.mic_data = rb_init("mic_data", dd.rb_size);
    if (dd.mic_data == NULL) {
        ESP_LOGE(TAG, "dd.mic_data rb_init failed!");
        return;
    }

#ifdef ENABLE_ESP_WWE
    dd.preroll_data = rb_init("preroll", PREROLL_RB_SIZE);
    if (dd.preroll_data == NULL) {
        ESP_LOGE(TAG, "dd.preroll_data rb_init failed!");
        return;
    }
    dd.va_queue = queue;

    if (esp_wwe_init() != ESP_OK) {
        ESP_LOGE(TAG, "Failed to init ESP-WWE");
        return;
    }

    xTaskCreate(&ww_detection_task, "ww_detection", dd.task_stack_size, NULL, (CONFIG_ESP32_PTHREAD_TASK_PRIO_DEFAULT - 1), &dd.ww_detection_task_handle);
    dd.preroll_status = PREROLL_IDLE;
    dd.detect_wakeword = true;
#endif

    return;
}
