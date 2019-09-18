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
#include <media_hal.h>
#include <voice_assistant.h>
#include <speech_recognizer.h>
#include <va_mem_utils.h>
#include <va_button.h>
#include <va_nvs_utils.h>
#include <va_dsp.h>
#include <lyrat_init.h>

#define AUDIO_BUF_SIZE (320)

#define EVENTQ_LENGTH   5
#define STACK_SIZE      (6 * 1024)
#define DSP_NVS_KEY "dsp_mute"

static const char *TAG = "[va_dsp]";

enum va_dsp_state {
    STREAMING,
    STOPPED,
    MUTED,
};
static enum va_dsp_state dsp_state;
static QueueHandle_t cmd_queue;
static uint8_t audio_buf[AUDIO_BUF_SIZE];
static bool va_dsp_booted = false;
static int8_t dsp_mute_en;

static inline void _va_dsp_stop_streaming()
{
    lyrat_stop_capture();
    dsp_state = STOPPED;
}

static inline void _va_dsp_start_streaming()
{
    lyrat_start_capture();
    dsp_state = STREAMING;
}

static inline int _va_dsp_stream_audio(uint8_t *buffer, int size, int wait)
{
    return lyrat_stream_audio(buffer, size, wait);
}

static inline void _va_dsp_mute_mic()
{
    if (dsp_state == STREAMING) {
        lyrat_stop_capture();
    }
    lyrat_mic_mute();
    dsp_state = MUTED;
}

static inline void _va_dsp_unmute_mic()
{
    lyrat_mic_unmute();
    dsp_state = STOPPED;
}

static void va_dsp_thread(void *arg)
{
    struct dsp_event_data event_data;
    while(1) {
        xQueueReceive(cmd_queue, &event_data, portMAX_DELAY);
        switch (dsp_state) {
            case STREAMING:
                switch (event_data.event) {
                    case TAP_TO_TALK:
                        /* Stop the streaming */
                        _va_dsp_stop_streaming();
                        break;
                    case GET_AUDIO: {
                        int read_len = _va_dsp_stream_audio(audio_buf, AUDIO_BUF_SIZE, portMAX_DELAY);
                        if (read_len > 0) {
                            speech_recognizer_record(audio_buf, read_len);
                            struct dsp_event_data new_event = {
                                .event = GET_AUDIO
                            };
                            xQueueSend(cmd_queue, &new_event, portMAX_DELAY);
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
                    case WW:
                    case START_MIC:
                    case UNMUTE:
                    default:
                        printf("%s: Event %d unsupported in STREAMING state\n", TAG, event_data.event);
                        break;
                }
                break;
            case STOPPED:
                switch (event_data.event) {
                    case WW: {
                        size_t phrase_length = 0;
                        if (phrase_length == 0) {
                            /*XXX: Should we close the stream here?*/
                            break;
                        }
                        if (speech_recognizer_recognize(phrase_length, WAKEWORD) == 0) {
                            struct dsp_event_data new_event = {
                                .event = GET_AUDIO
                            };
                            xQueueSend(cmd_queue, &new_event, portMAX_DELAY);
                            dsp_state = STREAMING;
                        } else {
                            printf("%s: Error starting a new dialog..stopping capture\n", TAG);
                            _va_dsp_stop_streaming();
                        }
                        break;
                    }
                    case TAP_TO_TALK:
                        if (speech_recognizer_recognize(0, TAP) == 0) {
                            _va_dsp_start_streaming();
                            struct dsp_event_data new_event = {
                                .event = GET_AUDIO
                            };
                            xQueueSend(cmd_queue, &new_event, portMAX_DELAY);
                        }
                        break;
                    case START_MIC:
                        _va_dsp_start_streaming();
                        struct dsp_event_data new_event = {
                            .event = GET_AUDIO
                        };
                        xQueueSend(cmd_queue, &new_event, portMAX_DELAY);
                        break;
                    case MUTE:
                        _va_dsp_mute_mic();
                        break;
                    case GET_AUDIO:
                    case STOP_MIC:
                    case UNMUTE:
                    default:
                        printf("%s: Event %d unsupported in STOPPED state\n", TAG, event_data.event);
                        break;
                }
                break;
            case MUTED:
                switch (event_data.event) {
                    case UNMUTE:
                        _va_dsp_unmute_mic();
                        break;
                    case WW:
                    case TAP_TO_TALK:
                    case GET_AUDIO:
                    case START_MIC:
                    case STOP_MIC:
                    case MUTE:
                    default:
                        printf("%s: Event %d unsupported in MUTE state\n", TAG, event_data.event);
                        break;
                }
                break;

            default:
                printf("%s: Unknown state %d with Event %d\n", TAG, dsp_state, event_data.event);
                break;
        }
    }
}

int va_app_speech_stop()
{
    printf("%s: Sending stop command\n", TAG);
    struct dsp_event_data new_event = {
        .event = STOP_MIC
    };
    xQueueSend(cmd_queue, &new_event, portMAX_DELAY);
    return 0;
}

int va_app_speech_start()
{
    printf("%s: Sending start speech command\n", TAG);
    struct dsp_event_data new_event = {
        .event = START_MIC
    };
    xQueueSend(cmd_queue, &new_event, portMAX_DELAY);
    return 0;
}

int va_dsp_tap_to_talk_start()
{
    if (va_dsp_booted == false) {
        return -1;
    }
    printf("%s: Sending start for tap to talk command\n", TAG);
    struct dsp_event_data new_event = {
        .event = TAP_TO_TALK
    };
    xQueueSend(cmd_queue, &new_event, portMAX_DELAY);
    return ESP_OK;
}

int va_app_playback_starting()
{
    return 0;
}

void va_dsp_reset()
{
    if (va_dsp_booted == true) {
        struct dsp_event_data new_event;
        new_event.event = MUTE;
        xQueueSend(cmd_queue, &new_event, portMAX_DELAY);
    }
}

void va_dsp_mic_mute(bool mute)
{
    struct dsp_event_data new_event;
    if (mute)
        new_event.event = MUTE;
    else
        new_event.event = UNMUTE;
    va_nvs_set_i8(DSP_NVS_KEY, mute);
    xQueueSend(cmd_queue, &new_event, portMAX_DELAY);
}

void va_dsp_init(void)
{
    lyrat_init();
    TaskHandle_t xHandle = NULL;
    StackType_t *task_stack = (StackType_t *)va_mem_alloc(STACK_SIZE, VA_MEM_INTERNAL);
    static StaticTask_t task_buf;

    cmd_queue = xQueueCreate(10, sizeof(struct dsp_event_data));
    if (!cmd_queue) {
        ESP_LOGE(TAG, "Error creating va_dsp queue");
        return;
    }

    dsp_state = STOPPED;
    if (va_nvs_get_i8(DSP_NVS_KEY, &dsp_mute_en) == ESP_OK) {
        if (dsp_mute_en) {
            va_dsp_mic_mute(dsp_mute_en);
            va_button_notify_mute(dsp_mute_en);
        }
    }

    xHandle = xTaskCreateStatic(va_dsp_thread, "dspg-thread", STACK_SIZE,
                                NULL, CONFIG_ESP32_PTHREAD_TASK_PRIO_DEFAULT, task_stack, &task_buf);
    if (xHandle == NULL) {
        ESP_LOGE(TAG, "Couldn't create thead");
    }

    va_boot_dsp_signal();
    va_dsp_booted = true;
}
