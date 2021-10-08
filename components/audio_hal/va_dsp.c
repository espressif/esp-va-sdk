// Copyright 2018 Espressif Systems (Shanghai) PTE LTD
// All rights reserved.

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>
#include <esp_timer.h>
#include <esp_log.h>
#include <esp_pm.h>
#include <driver/i2s.h>

#include <esp_audio_mem.h>
#include <esp_audio_nvs.h>
#include <media_hal.h>

#include <audio_board.h>
#include <va_dsp.h>
#include <va_dsp_hal.h>

#define AUDIO_BUF_SIZE 4096
#define EVENTQ_LENGTH   10
#define STACK_SIZE      (6 * 1024)
#define DSP_NVS_KEY "dsp_mute"

static const char *TAG = "[va_dsp]";

static int8_t dsp_mute_en;

#ifdef CONFIG_PM_ENABLE
#define LOW_POWER_TIMER_SECS_SHORT 10       /* Go to low power after 10 seconds */
#define LOW_POWER_TIMER_SECS_LONG  60       /* Go to low power after 60 seconds */

static esp_timer_handle_t low_power_timer;
static esp_pm_lock_handle_t playback_pm;
#endif

enum va_dsp_state {
    STREAMING,
    STOPPED,
    MUTED,
};

static struct va_dsp_data_t {
    va_dsp_record_cb_t va_dsp_record_cb;
    va_dsp_recognize_cb_t va_dsp_recognize_cb;
    va_dsp_notify_mute_cb_t va_dsp_mute_notify_cb;
    enum va_dsp_state dsp_state;
    QueueHandle_t cmd_queue;
    TaskHandle_t xHandle;
    uint8_t *audio_buf;
    bool va_dsp_booted;
    bool low_power_enabled;
    bool first_playback_starting;
} va_dsp_data = {
    .va_dsp_record_cb = NULL,
    .va_dsp_recognize_cb = NULL,
    .xHandle = NULL,
    .va_dsp_booted = false,
};

#ifdef CONFIG_PM_ENABLE
static void va_dsp_low_power_enter()
{
    if (va_dsp_data.low_power_enabled == true) {
        ESP_LOGI(TAG, "Low power already enabled");
        return;
    }
    ESP_LOGI(TAG, "Entering DSP Low Power");
    va_dsp_hal_enter_low_power();

    PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO0_U, FUNC_GPIO0_GPIO0);
    gpio_set_level(GPIO_NUM_0, 0);
    va_dsp_data.low_power_enabled = true;
    esp_pm_lock_release(playback_pm);
}

static void va_dsp_low_power_exit()
{
    if (va_dsp_data.low_power_enabled == false) {
        ESP_LOGI(TAG, "Low power already disabled");
        return;
    }
    ESP_LOGI(TAG, "Exiting DSP Low Power");
    esp_pm_lock_acquire(playback_pm);
    PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO0_U, FUNC_GPIO0_CLK_OUT1);

    va_dsp_hal_exit_low_power();
    va_dsp_data.low_power_enabled = false;
}

static void va_dsp_low_power_start_timer(int time_is_sec)
{
    esp_timer_stop(low_power_timer);
    esp_timer_start_once(low_power_timer, time_is_sec * 1000 * 1000);
}

static void va_dsp_low_power_stop_timer()
{
    esp_timer_stop(low_power_timer);
}

static void low_power_timer_cb(void *arg)
{
    va_dsp_low_power_enter();
}
#endif

static inline void _va_dsp_stop_streaming()
{
    va_dsp_hal_stop_capture();
    va_dsp_data.dsp_state = STOPPED;
}

static inline void _va_dsp_start_streaming()
{
    va_dsp_hal_start_capture();
    va_dsp_data.dsp_state = STREAMING;
}

static inline void _va_dsp_mute_mic()
{
    if (va_dsp_data.dsp_state == STREAMING) {
        va_dsp_hal_stop_capture();
    }
    va_dsp_hal_mic_mute();
#ifdef CONFIG_PM_ENABLE
    va_dsp_low_power_start_timer(LOW_POWER_TIMER_SECS_SHORT);
#endif
    va_dsp_data.dsp_state = MUTED;
}

static inline void _va_dsp_unmute_mic()
{
    va_dsp_hal_mic_unmute();
    va_dsp_data.dsp_state = STOPPED;
}

static void va_dsp_thread(void *arg)
{
    struct dsp_event_data event_data;
    while(1) {
        xQueueReceive(va_dsp_data.cmd_queue, &event_data, portMAX_DELAY);
        switch (va_dsp_data.dsp_state) {
            case STREAMING:
                switch (event_data.event) {
                    case TAP_TO_TALK:
                        /* Stop the streaming */
                        _va_dsp_stop_streaming();
                        break;
                    case GET_AUDIO: {
                        int read_len = va_dsp_hal_stream_audio(va_dsp_data.audio_buf, AUDIO_BUF_SIZE, portMAX_DELAY);
                        if (read_len > 0) {
                            va_dsp_data.va_dsp_record_cb(va_dsp_data.audio_buf, read_len);
                            struct dsp_event_data new_event = {
                                .event = GET_AUDIO
                            };
                            xQueueSend(va_dsp_data.cmd_queue, &new_event, portMAX_DELAY);
                        } else {
                            _va_dsp_stop_streaming();
                        }
                        break;
                    }
                    case STOP_MIC:
                        _va_dsp_stop_streaming();
                        break;
                    case MUTE:
                        _va_dsp_mute_mic();
                        break;
                    case POWER_SAVE:
                    case WW:
                    case START_MIC:
                    case UNMUTE:
                    default:
                        ESP_LOGI(TAG, "Event %d unsupported in STREAMING state", event_data.event);
                        break;
                }
                break;
            case STOPPED:
                switch (event_data.event) {
                    case WW: {
                        size_t phrase_length = va_dsp_hal_get_ww_len();
                        if (phrase_length == 0) {
                            /*XXX: Should we close the stream here?*/
                            ESP_LOGE(TAG, "WW detected but length is 0. Not processing.");
                            break;
                        }
                        if (va_dsp_data.va_dsp_recognize_cb(phrase_length, WAKEWORD) == 0) {
                            _va_dsp_start_streaming();
                            struct dsp_event_data new_event = {
                                .event = GET_AUDIO
                            };
                            xQueueSend(va_dsp_data.cmd_queue, &new_event, portMAX_DELAY);
                            va_dsp_data.dsp_state = STREAMING;
                        } else {
                            ESP_LOGI(TAG, "Error starting a new dialog..stopping capture");
                            va_dsp_hal_stop_capture();
                        }
                        break;
                    }
                    case TAP_TO_TALK:
                        if (va_dsp_data.va_dsp_recognize_cb(0, TAP) == 0) {
                            _va_dsp_start_streaming();
                            struct dsp_event_data new_event = {
                                .event = GET_AUDIO
                            };
                            xQueueSend(va_dsp_data.cmd_queue, &new_event, portMAX_DELAY);
                        } else {
                            ESP_LOGI(TAG, "Error starting a new dialog");
                        }
                        break;
                    case START_MIC:
                        _va_dsp_start_streaming();
                        struct dsp_event_data new_event = {
                            .event = GET_AUDIO
                        };
                        xQueueSend(va_dsp_data.cmd_queue, &new_event, portMAX_DELAY);
                        break;
                    case MUTE:
                        _va_dsp_mute_mic();
                        break;
                    case POWER_SAVE:
                    case GET_AUDIO:
                    case STOP_MIC:
                    case UNMUTE:
                    default:
                        ESP_LOGI(TAG, "Event %d unsupported in STOPPED state", event_data.event);
                        break;
                }
                break;
            case MUTED:
                switch (event_data.event) {
                    case UNMUTE:
                        _va_dsp_unmute_mic();
                        break;
                    case POWER_SAVE:
                    case WW:
                    case TAP_TO_TALK:
                    case GET_AUDIO:
                    case START_MIC:
                    case STOP_MIC:
                    case MUTE:
                    default:
                        ESP_LOGI(TAG, "Event %d unsupported in MUTE state", event_data.event);
                        break;
                }
                break;

            default:
                ESP_LOGI(TAG, "Unknown state %d with Event %d", va_dsp_data.dsp_state, event_data.event);
                break;
        }
    }
}

int va_app_speech_stop()
{
    if (!va_dsp_data.va_dsp_booted) {
        ESP_LOGE(TAG, "Not initialized yet");
        return -1;
    }
    ESP_LOGI(TAG, "Sending stop command");
    struct dsp_event_data new_event = {
        .event = STOP_MIC
    };
    xQueueSend(va_dsp_data.cmd_queue, &new_event, portMAX_DELAY);
    return 0;
}

int va_app_speech_start()
{
    if (!va_dsp_data.va_dsp_booted) {
        ESP_LOGE(TAG, "Not initialized yet");
        return -1;
    }
    ESP_LOGI(TAG, "Sending start speech command");
    struct dsp_event_data new_event = {
        .event = START_MIC
    };
    xQueueSend(va_dsp_data.cmd_queue, &new_event, portMAX_DELAY);
    return 0;
}

int va_dsp_playback_starting()
{
    if (!va_dsp_data.va_dsp_booted) {
        ESP_LOGD(TAG, "Not initialized yet");
        return -1;
    }
#ifdef CONFIG_HALF_DUPLEX_I2S_MODE
    if (!va_dsp_data.first_playback_starting) {
        va_dsp_data.first_playback_starting = true;
    }
    audio_board_i2s_set_spk_mic_mode(MODE_SPK);
#endif
#ifdef CONFIG_PM_ENABLE
    va_dsp_low_power_stop_timer();
    va_dsp_low_power_exit();
    va_dsp_low_power_start_timer(LOW_POWER_TIMER_SECS_LONG);
#endif
    return 0;
}

int va_dsp_playback_ongoing()
{
    if (!va_dsp_data.va_dsp_booted) {
        ESP_LOGD(TAG, "Not initialized yet");
        return -1;
    }
#ifdef CONFIG_PM_ENABLE
    if (va_dsp_data.low_power_enabled) {
        va_dsp_low_power_exit();
    } else {
        va_dsp_low_power_start_timer(LOW_POWER_TIMER_SECS_LONG);
    }
#endif
    return 0;
}

int va_dsp_playback_stopped()
{
    if (!va_dsp_data.va_dsp_booted) {
        ESP_LOGD(TAG, "Not initialized yet");
        return -1;
    }
#ifdef CONFIG_HALF_DUPLEX_I2S_MODE
    audio_board_i2s_set_spk_mic_mode(MODE_MIC);
#endif
#ifdef CONFIG_PM_ENABLE
    va_dsp_low_power_start_timer(LOW_POWER_TIMER_SECS_SHORT);
#endif
    return 0;
}

int va_dsp_tap_to_talk_start()
{
    if (!va_dsp_data.va_dsp_booted) {
        ESP_LOGE(TAG, "Not initialized yet");
        return -1;
    }
    ESP_LOGI(TAG, "Sending event for tap to talk command");
    struct dsp_event_data new_event = {
        .event = TAP_TO_TALK
    };
    xQueueSend(va_dsp_data.cmd_queue, &new_event, portMAX_DELAY);
    return 0;
}

void va_dsp_reset()
{
    if (!va_dsp_data.va_dsp_booted) {
        ESP_LOGD(TAG, "Not initialized yet");
        return;
    }
    va_dsp_hal_reset();
}

void va_dsp_mic_mute(bool mute)
{
    if (!va_dsp_data.va_dsp_booted) {
        ESP_LOGE(TAG, "Not initialized yet");
        return;
    }
    struct dsp_event_data new_event;
    if (mute)
        new_event.event = MUTE;
    else
        new_event.event = UNMUTE;
    esp_audio_nvs_set_i8(DSP_NVS_NAMESPACE, DSP_NVS_KEY, mute);
    xQueueSend(va_dsp_data.cmd_queue, &new_event, portMAX_DELAY);
}

void va_dsp_init(va_dsp_recognize_cb_t va_dsp_recognize_cb, va_dsp_record_cb_t va_dsp_record_cb, va_dsp_notify_mute_cb_t va_dsp_mute_notify_cb)
{
    va_dsp_data.va_dsp_record_cb = va_dsp_record_cb;
    va_dsp_data.va_dsp_recognize_cb = va_dsp_recognize_cb;
    va_dsp_data.va_dsp_mute_notify_cb = va_dsp_mute_notify_cb;

    StackType_t *task_stack = (StackType_t *)heap_caps_calloc(1, STACK_SIZE, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    static StaticTask_t task_buf;

    va_dsp_data.cmd_queue = xQueueCreate(EVENTQ_LENGTH, sizeof(struct dsp_event_data));
    if (!va_dsp_data.cmd_queue) {
        ESP_LOGE(TAG, "Error creating va_dsp queue");
        return;
    }

    va_dsp_data.audio_buf = heap_caps_calloc(1, AUDIO_BUF_SIZE, MALLOC_CAP_DMA);
    if (!va_dsp_data.audio_buf) {
        ESP_LOGE(TAG, "Error allocating audio buffer in DMA region");
        return;
    }

#ifdef CONFIG_HALF_DUPLEX_I2S_MODE
    /* First set I2S into Mic mode and then start audio stream, otherwise it fails in i2c_read RX NULL loop */
    if (!va_dsp_data.first_playback_starting) {
        audio_board_i2s_set_spk_mic_mode(MODE_MIC);
    }
#endif

    va_dsp_hal_init(va_dsp_data.cmd_queue);

#ifdef CONFIG_PM_ENABLE
    esp_pm_lock_create(ESP_PM_APB_FREQ_MAX, 0, "Playback Lock", &playback_pm);
    esp_timer_create_args_t timer_arg = {
        .callback = low_power_timer_cb,
        .arg = NULL,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "va_dsp_low_power",
    };
    if (esp_timer_create(&timer_arg, &low_power_timer) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create low power timer");
    }
    va_dsp_low_power_enter();
#endif

    va_dsp_data.dsp_state = STOPPED;
    //Check if device was Mute or Unmute before power off / reboot and set the last state device was in
    if (esp_audio_nvs_get_i8(DSP_NVS_NAMESPACE, DSP_NVS_KEY, &dsp_mute_en) == ESP_OK) {
        if (dsp_mute_en) {
            va_dsp_mic_mute(dsp_mute_en);
            va_dsp_data.va_dsp_mute_notify_cb(dsp_mute_en);
        }
    }

    va_dsp_data.xHandle = xTaskCreateStatic(va_dsp_thread, "va_dsp", STACK_SIZE,
                                NULL, CONFIG_ESP32_PTHREAD_TASK_PRIO_DEFAULT, task_stack, &task_buf);
    if (va_dsp_data.xHandle == NULL) {
        ESP_LOGE(TAG, "Couldn't create thead");
    }

    va_dsp_data.va_dsp_booted = true;
}
