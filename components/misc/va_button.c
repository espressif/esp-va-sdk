// Copyright 2018 Espressif Systems (Shanghai) PTE LTD
// All rights reserved.

#include <freertos/FreeRTOS.h>
#include <freertos/timers.h>
#include "va_button.h"
#include <media_hal.h>
#include <va_mem_utils.h>
#include "speaker.h"
#include <alerts.h>
#include <tone.h>
#include <voice_assistant.h>
#include "va_dsp.h"
#include "va_nvs_utils.h"
#include "va_ui.h"

#include <button_driver.h>

#define VOLUME_STEP 5
#define VOL_NOTIF_DELAY 850
#define FACTORY_RST_DELAY 10000
#define WIFI_RST_DELAY 5000

static const char *TAG = "[va_button]";
static bool va_button_wifi_rst_en = false;
static bool va_button_factory_rst_en = false;
static bool va_button_is_wifi_rst = false;
static bool va_button_is_factory_rst = false;
static va_button_factory_reset_cb_t va_button_factory_reset_cb = NULL;
static va_button_wifi_reset_cb_t va_button_wifi_reset_cb = NULL;
static va_button_setup_mode_cb_t va_button_setup_mode_cb = NULL;
static uint8_t mute_btn_press_flg = 1;

static int set_volume = 0;

typedef struct {
    bool b_mute;
    esp_timer_handle_t esp_timer_handler_f;
    esp_timer_handle_t esp_timer_handler_w;
} va_button_t;

static va_button_t va_button = {false, NULL, NULL};

int number_of_active_alerts = 0;

static void va_button_factory_reset_timer_cb()
{
    va_button_factory_rst_en = true;
    ESP_LOGI(TAG, "Factory Reset initiated, erasing nvs flash and rebooting");
}

static void va_button_wifi_reset_timer_cb()
{
    va_button_wifi_rst_en = true;
    ESP_LOGI(TAG, "Wifi Reset initiated");
}

void va_button_notify_mute(bool va_btn_mute)
{
    if(va_btn_mute) {
        mute_btn_press_flg = 3;
        va_ui_set_state(VA_MUTE_ENABLE);
    }
}

static void va_button_task(void *arg)
{
    bool en_timer = 0;
    int64_t curr_time = esp_timer_get_time();
    uint8_t current_vol;
    bool active_btn_press = false;
    bool act_btn_press_en = false;
    button_event_t event = BUTTON_EVENT_MAX;

    while (1) {
        event = button_driver_get_event();

        if(event == BUTTON_EVENT_TAP_TO_TALK) {
            esp_timer_start_once(va_button.esp_timer_handler_w, (WIFI_RST_DELAY * 1000));
            act_btn_press_en = true;
        } else if(event == BUTTON_EVENT_MIC_MUTE) {
            if(va_boot_is_finish()) {
                if(mute_btn_press_flg == 1) {
                    va_button.b_mute = true;
                    mute_btn_press_flg = 2;
                } else if(mute_btn_press_flg == 3) {
                    mute_btn_press_flg = 4;
                    va_button.b_mute = false;
                }
            }
        } else if(event == BUTTON_EVENT_VOLUME_UP) {
            if(va_boot_is_finish()) {
                media_hal_get_volume(media_hal_get_handle(), &current_vol);
                set_volume = current_vol + VOLUME_STEP;
                if (set_volume > 100) {
                    set_volume = 100;
                    ESP_LOGI(TAG, "volume_max");
                }
                media_hal_set_mute(media_hal_get_handle(), 0);
                volume_to_set = set_volume;
                va_ui_set_state(VA_SET_VOLUME);
                media_hal_control_volume(media_hal_get_handle(), set_volume);
                /* It is good if we play some tone (when nothing else is playing). But there is no dedicated tone for volume. So putting our custom tone. */
                tone_play(TONE_VOLUME);
                curr_time = esp_timer_get_time();
                en_timer = true;
            }
        } else if(event == BUTTON_EVENT_VOLUME_DOWN) {
            if(va_boot_is_finish()) {
                media_hal_get_volume(media_hal_get_handle(), &current_vol);
                set_volume = current_vol - VOLUME_STEP;
                if (set_volume < 0) {
                    set_volume = 0;
                    ESP_LOGI(TAG, "volume_min");
                }
                media_hal_set_mute(media_hal_get_handle(), 0);
                volume_to_set = set_volume;
                va_ui_set_state(VA_SET_VOLUME);
                media_hal_control_volume(media_hal_get_handle(), set_volume);
                /* It is good if we play some tone (when nothing else is playing). But there is no dedicated tone for volume. So putting our custom tone. */
                tone_play(TONE_VOLUME);
                curr_time = esp_timer_get_time();
                en_timer = true;
            }
        } else if(event == BUTTON_EVENT_FACTORY_RST) {
            esp_timer_stop(va_button.esp_timer_handler_w);
            esp_timer_start_once(va_button.esp_timer_handler_f, (FACTORY_RST_DELAY * 1000));
        } else if(event == BUTTON_EVENT_CUSTOM_1) {
            /* Do nothing */
        } else if(event == BUTTON_EVENT_CUSTOM_2) {
            /* Do nothing */
        } else if(event == BUTTON_EVENT_IDLE) {
            esp_timer_stop(va_button.esp_timer_handler_w);  //Stop wifi reset timer
            esp_timer_stop(va_button.esp_timer_handler_f);  //Stop factory reset timer
            if(act_btn_press_en) {
                active_btn_press = true;
            }
            if(mute_btn_press_flg == 2) {
                va_dsp_mic_mute(va_button.b_mute);
                va_ui_set_state(VA_MUTE_ENABLE);
                tone_play(TONE_PRIVACY_ON);
                mute_btn_press_flg = 3;
            } else if (mute_btn_press_flg == 4) {
                va_dsp_mic_mute(va_button.b_mute);
                va_ui_set_state(VA_MUTE_DISABLE);
                tone_play(TONE_PRIVACY_OFF);
                mute_btn_press_flg = 1;
            }
        }

        if (((esp_timer_get_time() - curr_time) > (VOL_NOTIF_DELAY * 1000)) && en_timer) {
            speaker_notify_vol_changed(set_volume);
            va_ui_set_state(VA_SET_VOLUME_DONE);
            en_timer = false;
        }
        if(va_button_wifi_rst_en) {
            if (va_button_wifi_reset_cb == NULL) {
                ESP_LOGE(TAG, "No callback set for wifi reset");
            } else {
                /* Do not set any led here. Instead set the Factory reset led when starting provisioning after restart. */
                va_ui_set_state(VA_UI_OFF);
                va_reset(false);
                (*va_button_wifi_reset_cb)(NULL);
                va_button_is_wifi_rst = true;   //Since non-blocking as of now
            }
            va_button_wifi_rst_en = false;
        }
        if(va_button_factory_rst_en) {
            va_button_is_factory_rst = true;   //Lets keep for it, can be removed as this is a blocking call
            /* Do not set any led here. Instead set the Factory reset led when starting provisioning after restart. */
            va_ui_set_state(VA_UI_OFF);
            va_nvs_flash_erase();
            va_reset(true);
            if (va_button_factory_reset_cb) {
                (*va_button_factory_reset_cb)(NULL);
            }
            esp_restart();
            va_button_factory_rst_en = false;
        }
        if(active_btn_press && act_btn_press_en && !(va_button_factory_rst_en) && !(va_button_is_wifi_rst)) {
            printf("%s: Tap to talk button pressed\n", TAG);
            if (number_of_active_alerts > 0) {
#if defined(VOICE_ASSISTANT_AVS) || defined(VOICE_ASSISTANT_AIA)
                if (alerts_is_active() == true) {
                    alerts_stop_currently_active();
                }
#endif /* VOICE_ASSISTANT_AVS || VOICE_ASSISTANT_AIA */
            } else {
                if (!va_boot_is_finish()) {
                    if (va_button_setup_mode_cb != NULL) {
                        (*va_button_setup_mode_cb)(NULL);
                    }
                } else if (!(va_button.b_mute)) {
                    va_dsp_tap_to_talk_start();
                }
            }
            active_btn_press = false;
            act_btn_press_en = false;
        }
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}

void va_button_register_factory_reset_cb(va_button_factory_reset_cb_t factory_reset_cb)
{
    va_button_factory_reset_cb = factory_reset_cb;
}

void va_button_register_wifi_reset_cb(va_button_wifi_reset_cb_t wifi_reset_cb)
{
    va_button_wifi_reset_cb = wifi_reset_cb;
}

void va_button_register_setup_mode_cb(va_button_setup_mode_cb_t setup_mode_cb)
{
    va_button_setup_mode_cb = setup_mode_cb;
}

esp_err_t va_button_init()
{
    if (button_driver_is_init_done() == false) {
        ESP_LOGE(TAG, "Button driver has not been initialized yet. Make sure that is initialized before calling va_button_init()");
        return ESP_FAIL;
    }
    /* Set timers for factory reset and wifi reset */
    esp_timer_init();
    esp_timer_create_args_t timer_arg = {
        .dispatch_method = ESP_TIMER_TASK,
        .arg = NULL,
    };

    /* Factory reset timer */
    timer_arg.callback = va_button_factory_reset_timer_cb;
    timer_arg.name = "va button factory reset timer";
    if (esp_timer_create(&timer_arg, &va_button.esp_timer_handler_f) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create esp timer for reset-to-factory button");
        return ESP_FAIL;
    }

    /* Wifi reset timer */
    timer_arg.callback = va_button_wifi_reset_timer_cb;
    timer_arg.name = "va button wifi reset timer";
    if (esp_timer_create(&timer_arg, &va_button.esp_timer_handler_w) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create esp timer for reset-to-wifi button");
        return ESP_FAIL;
    }

    /* Start the va_button task */
#define VA_BUTTON_STACK (8 * 1024)
    StackType_t *task_stack = (StackType_t *)va_mem_alloc(VA_BUTTON_STACK, VA_MEM_EXTERNAL);
    static StaticTask_t va_button_task_buf;
    TaskHandle_t task_handle = xTaskCreateStatic(va_button_task, "va-button-thread", VA_BUTTON_STACK, NULL, CONFIG_ESP32_PTHREAD_TASK_PRIO_DEFAULT, task_stack, &va_button_task_buf);
    if (task_handle == NULL) {
        ESP_LOGE(TAG, "Could not create va button task");
        return ESP_FAIL;
    }

    return ESP_OK;
}
