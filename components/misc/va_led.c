// Copyright 2018 Espressif Systems (Shanghai) PTE LTD
// All rights reserved.

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include <va_mem_utils.h>
#include <va_ui.h>
#include <led_driver.h>
#include <led_pattern.h>
#include <va_led.h>
#include <esp_timer.h>

//#define EN_STACK_MEASUREMENT
#define BOOTUP_MAX_TIME_SEC 300     /* 5 minutes */

static const char *TAG = "[va_led]";

static int NOTIFICATION_IS_PRESENT = 0;
static int ALERT_IS_PRESENT = 0;
static int DND_IS_PRESENT = 0;
static bool va_led_notif_incoming_is_done = false;
static bool va_led_dnd_st = false;
static bool va_led_error_st = false;
static bool va_led_alert_short_en = false;
static bool init_done;
static uint8_t counter = 0;
static uint64_t bootup_start_time = 0;

typedef struct {
    TaskHandle_t va_led_task_handle;
    SemaphoreHandle_t va_led_tim_sema;
    SemaphoreHandle_t va_led_patttern_sema;
    esp_timer_handle_t esp_delay_timer_hdl;
} va_led_t;

static va_led_t led_st = {NULL, NULL,  NULL, NULL};

typedef struct {
    uint32_t va_led_current_state;
    bool va_led_priority_status;
} va_led_priority_t;
static va_led_priority_t va_led_priority[4];    //Mute, Unmute, Vol, all other states

static void va_led_bootup_start()
{
    bootup_start_time = esp_timer_get_time() / (1000 * 1000);
}

static void va_led_bootup_expired()
{
    va_ui_set_state(VA_IDLE);
}

static bool va_led_bootup_check_time_expiry()
{
    uint64_t current_time = esp_timer_get_time() / (1000 * 1000);
    bool ret = (current_time - bootup_start_time) > BOOTUP_MAX_TIME_SEC;
    if (ret == true) {
        va_led_bootup_expired();
    }
    return ret;
}

void va_led_delay_timer_cb()
{
    xSemaphoreGive(led_st.va_led_tim_sema);
}

IRAM_ATTR esp_err_t va_led_set(int va_state)
{
    if (!init_done) {
        return ESP_OK;
    }
    if(va_state == VA_MUTE_ENABLE) {
        va_led_alert_short_en = false;
        va_led_priority[0].va_led_priority_status = true;
        va_led_priority[1].va_led_priority_status = false;
        va_led_priority[0].va_led_current_state = va_state;
    } else if (va_state == VA_MUTE_DISABLE) {
        va_led_alert_short_en = false;
        va_led_priority[1].va_led_priority_status = true;
        va_led_priority[0].va_led_priority_status = false;
        va_led_priority[1].va_led_current_state = va_state;
        if (DND_IS_PRESENT) {
            va_led_dnd_st = true;
        }
    } else if (va_state == VA_SET_VOLUME || va_state == VA_SPEAKER_MUTE_ENABLE) {
        va_led_alert_short_en = false;
        va_led_priority[2].va_led_priority_status = true;
        va_led_priority[2].va_led_current_state = va_state;
        va_led_priority[3].va_led_priority_status = false; //When in volume condition, turn off other states
    } else if (va_state == VA_SET_VOLUME_DONE) {
        va_led_priority[3].va_led_priority_status = true;
    } else if (va_state == VA_UI_ERROR) {
        va_led_error_st = true;
    } else if (va_state == LED_PATTERN_ALERT_SHORT) {
        counter = 0;
        va_led_alert_short_en = true;
    } else {
        va_led_priority[3].va_led_priority_status = true;    //Once in this state, this will always be true
        va_led_priority[3].va_led_current_state = va_state;
    }

#ifdef EN_STACK_MEASUREMENT
    ESP_LOGI("TAG", "Free Task Stack is: %s %u\n\n\n", __func__, uxTaskGetStackHighWaterMark(led_st.va_led_task_handle));
#endif
    xSemaphoreGive(led_st.va_led_patttern_sema);
    if (!va_led_alert_short_en) {
        counter = 0;
    }
    return ESP_OK;
}

static void va_led_send_vl(led_pattern_state_t *va_led_v, uint8_t num)
{
    xSemaphoreTake(led_st.va_led_tim_sema, portMAX_DELAY);
    led_driver_set_value((uint32_t *)va_led_v[num].led_state_val);
    esp_timer_start_once(led_st.esp_delay_timer_hdl, (va_led_v[num].led_state_delay * 1000));
}

static void va_led_set_state(size_t sz, const led_pattern_state_t *va_led_conf)
{
    for(int i = 0; i < sz; i++) {
        va_led_send_vl((led_pattern_state_t *)va_led_conf, i);
    }
}

static void va_led_task(void *arg)
{
    bool va_led_is_mute = false;
    bool va_led_listen_on_going = false;
    bool va_led_listening_end_flag = false;
    led_pattern_config_t *va_led_con = (led_pattern_config_t *)arg;
    TickType_t va_led_tick = portMAX_DELAY;
    while (1) {
        vTaskDelay(1);      // Temporary fix for ui-led-thread spinning
        xSemaphoreTake(led_st.va_led_patttern_sema, va_led_tick);
        //Handle Mute
        if (va_led_priority[0].va_led_priority_status) {
            va_led_set_state(va_led_con[LED_PATTERN_MIC_OFF_ENTER].led_states_count, va_led_con[LED_PATTERN_MIC_OFF_ENTER].led_states);
            va_led_is_mute = true;
            va_led_priority[0].va_led_priority_status = false;
            va_led_tick = portMAX_DELAY;
        }
        //Handle Un-mute
        if (va_led_priority[1].va_led_priority_status) {
            va_led_set_state(va_led_con[LED_PATTERN_MIC_OFF_EXIT].led_states_count, va_led_con[LED_PATTERN_MIC_OFF_EXIT].led_states);
            va_led_is_mute = false;
            va_led_priority[1].va_led_priority_status = false;
            va_led_tick = portMAX_DELAY;
        }
        //Handle Volume and speaker Mute
        if (va_led_priority[2].va_led_priority_status) {
            if(volume_to_set == 0 || va_led_priority[2].va_led_current_state == VA_SPEAKER_MUTE_ENABLE) {
                for(int i = 0; i < 3; i++) {
                    va_led_set_state(va_led_con[LED_PATTERN_SPEAKER_MUTE].led_states_count, va_led_con[LED_PATTERN_SPEAKER_MUTE].led_states);
                }
            } else {
                uint8_t vol = volume_to_set / 5;
                if (vol <= 0) {
                    vol = 1;
                }
                va_led_send_vl(va_led_con[LED_PATTERN_SPEAKER_VOL].led_states, vol - 1);
                vTaskDelay(178 / portTICK_RATE_MS); //Need to figue out this delay
            }
            va_led_priority[2].va_led_priority_status = false;
        }
        //Handle all other led states
        if (va_led_priority[3].va_led_priority_status) {
            switch (va_led_priority[3].va_led_current_state) {
                case VA_UI_CAN_START :
                    va_led_bootup_start();
                    va_led_set_state(va_led_con[LED_PATTERN_BOOTUP_1].led_states_count, va_led_con[LED_PATTERN_BOOTUP_1].led_states);  //bootup 1
                    /* Stop when LEDs are turned off OR there is an error OR bootup time has expired */
                    while (va_led_priority[3].va_led_current_state != VA_UI_OFF && va_led_error_st != true && va_led_bootup_check_time_expiry() == false) {
                        va_led_send_vl(va_led_con[LED_PATTERN_BOOTUP_2].led_states, counter);
                        counter = (counter + 1) % va_led_con[LED_PATTERN_BOOTUP_2].led_states_count;
                    }
                    va_led_tick = portMAX_DELAY;
                break;
                case VA_IDLE :
                    if(va_led_listening_end_flag && (va_led_priority[3].va_led_current_state == VA_IDLE)) {
                        va_led_set_state(va_led_con[LED_PATTERN_LISTENING_EXIT].led_states_count, va_led_con[LED_PATTERN_LISTENING_EXIT].led_states);
                        va_led_listening_end_flag = false;
                        va_led_listen_on_going = false;
                    }
                    if (va_led_dnd_st) {
                        va_led_set_state(va_led_con[LED_PATTERN_DO_NOT_DISTURB].led_states_count, va_led_con[LED_PATTERN_DO_NOT_DISTURB].led_states);
                        va_led_dnd_st = false;
                    }
                    if (va_led_error_st) {
                        va_led_set_state(va_led_con[LED_PATTERN_ERROR].led_states_count, va_led_con[LED_PATTERN_ERROR].led_states);
                        va_led_error_st = false;
                    }
                    if (NOTIFICATION_IS_PRESENT && (!(ALERT_IS_PRESENT > 0) || va_led_notif_incoming_is_done == true)) {
                        if(va_led_notif_incoming_is_done) {
                            va_led_set_state(va_led_con[LED_PATTERN_NOTIFICATION_NEW].led_states_count, va_led_con[LED_PATTERN_NOTIFICATION_NEW].led_states);
                            va_led_notif_incoming_is_done = false;
                        }
                        if (!(ALERT_IS_PRESENT > 0)) {
                            va_led_send_vl(va_led_con[LED_PATTERN_NOTIFICATION_ONGOING].led_states, counter);
                            counter = (counter + 1) % va_led_con[LED_PATTERN_NOTIFICATION_ONGOING].led_states_count;
                            va_led_tick = 0;
                        }
                    } else if (ALERT_IS_PRESENT > 0) {
                        va_led_send_vl(va_led_con[LED_PATTERN_ALERT].led_states, counter);
                        counter = (counter + 1) % va_led_con[LED_PATTERN_ALERT].led_states_count;
                        va_led_tick = 0;
                    } else if(va_led_is_mute) {
                        va_led_set_state(va_led_con[LED_PATTERN_MIC_OFF_ONGOING].led_states_count, va_led_con[LED_PATTERN_MIC_OFF_ONGOING].led_states);
                        va_led_tick = portMAX_DELAY;
                        break;
                    } else {
                        va_led_set_state(va_led_con[LED_PATTERN_OFF].led_states_count, va_led_con[LED_PATTERN_OFF].led_states);  //LED off
                        va_led_tick = portMAX_DELAY;
                    }
                break;
                case VA_LISTENING :
                    va_led_listening_end_flag = true;
                    if (va_led_listen_on_going == false) {
                        va_led_set_state(va_led_con[LED_PATTERN_LISTENING_ENTER].led_states_count, va_led_con[LED_PATTERN_LISTENING_ENTER].led_states);
                        va_led_listen_on_going = true;
                        va_led_alert_short_en = false;
                    }
                    if (va_led_alert_short_en) {
                        va_led_send_vl(va_led_con[LED_PATTERN_ALERT_SHORT].led_states, counter);
                        if ((counter = (counter + 1) % va_led_con[LED_PATTERN_ALERT_SHORT].led_states_count) == 0) {
                            va_led_alert_short_en = false;
                        }
                        va_led_tick = 0;
                    } else {
                        va_led_set_state(va_led_con[LED_PATTERN_LISTENING_ONGOING].led_states_count, va_led_con[LED_PATTERN_LISTENING_ONGOING].led_states);
                        va_led_tick = portMAX_DELAY;
                    }
                break;
                case VA_THINKING :
                    va_led_listening_end_flag = true;
                    va_led_listen_on_going = false;
                    if (va_led_alert_short_en) {
                        va_led_send_vl(va_led_con[LED_PATTERN_ALERT_SHORT].led_states, counter);
                        if ((counter = (counter + 1) % va_led_con[LED_PATTERN_ALERT_SHORT].led_states_count) == 0) {
                            va_led_alert_short_en = false;
                        }
                    } else {
                        va_led_send_vl(va_led_con[LED_PATTERN_THINKING].led_states, counter);
                        counter = (counter + 1) % va_led_con[LED_PATTERN_THINKING].led_states_count;
                    }
                    va_led_tick = 0;
                break;
                case VA_SPEAKING :
                    va_led_listening_end_flag = true;
                    va_led_listen_on_going = false;
                    if (va_led_alert_short_en) {
                        va_led_send_vl(va_led_con[LED_PATTERN_ALERT_SHORT].led_states, counter);
                        if ((counter = (counter + 1) % va_led_con[LED_PATTERN_ALERT_SHORT].led_states_count) == 0) {
                            va_led_alert_short_en = false;
                        }
                    } else {
                        va_led_send_vl(va_led_con[LED_PATTERN_SPEAKING].led_states, counter);
                        counter = (counter + 1) % va_led_con[LED_PATTERN_SPEAKING].led_states_count;
                    }
                    va_led_tick = 0;
                    if (DND_IS_PRESENT) {
                        va_led_dnd_st = true;
                    }
                    break;
                case VA_UI_RESET :
                    va_led_send_vl(va_led_con[LED_PATTERN_SETUP].led_states, counter);
                    counter = (counter + 1) % va_led_con[LED_PATTERN_SETUP].led_states_count;
                    va_led_tick = 0;
                break;
                case VA_UI_OTA :
                    va_led_set_state(va_led_con[LED_PATTERN_OTA].led_states_count, va_led_con[LED_PATTERN_OTA].led_states);  //led off
                    va_led_tick = portMAX_DELAY;
                break;
                case VA_UI_OFF :
                    va_led_set_state(va_led_con[LED_PATTERN_OFF].led_states_count, va_led_con[LED_PATTERN_OFF].led_states);  //led off
                    va_led_tick = portMAX_DELAY;
                break;
                default :
                    va_led_tick = portMAX_DELAY;
                break;
            }
        }
    }
}

void va_led_set_alert(alexa_alert_types_t alert_type, alexa_alert_state_t alert_state)
{
    switch (alert_type) {
        case ALEXA_ALERT_NOTIFICATION:
            if (alert_state == ALEXA_ALERT_ENABLE) {
                NOTIFICATION_IS_PRESENT = 1;
                va_led_notif_incoming_is_done = true;
            } else if (alert_state == ALEXA_ALERT_DISABLE) {
                NOTIFICATION_IS_PRESENT = 0;
            }
            break;
        case ALEXA_ALERT_ALARM:
        case ALEXA_ALERT_REMINDER:
        case ALEXA_ALERT_TIMER:
            if (alert_state == ALEXA_ALERT_ENABLE) {
                ALERT_IS_PRESENT++;
            } else if (alert_state == ALEXA_ALERT_DISABLE) {
                ALERT_IS_PRESENT--;
                if (ALERT_IS_PRESENT <= 0) {
                    ALERT_IS_PRESENT = 0;
                    if (DND_IS_PRESENT) {
                        va_led_dnd_st = true;
                    }
                }
            }    
            break;
        default:
            break;
    }
}

void va_led_set_dnd(bool dnd_state)
{
    DND_IS_PRESENT = dnd_state;
}

esp_err_t va_led_init()
{
    if (led_driver_is_init_done() == false || led_pattern_is_init_done() == false) {
        ESP_LOGE(TAG, "LED driver/LED pattern has not been initialized yet. Make sure that is initialized before calling va_led_init()");
        return ESP_FAIL;
    }
    led_pattern_config_t *va_led_conf = NULL;
    led_pattern_get_config(&va_led_conf);

    static StaticTask_t va_led_buf;
    init_done = true;
    esp_err_t ret = ESP_FAIL;

    StackType_t *va_led_task_stack = (StackType_t *)va_mem_alloc(VA_LED_TASK_STACK_SZ, VA_MEM_EXTERNAL);
    if (va_led_task_stack == NULL) {
        ESP_LOGE(TAG, "Could not allocate memomory for ui led thread");
        return ESP_FAIL;
    }
    vSemaphoreCreateBinary(led_st.va_led_tim_sema);
    if (led_st.va_led_tim_sema == NULL) {
        ESP_LOGE(TAG, "Could not create semaphore");
        return ESP_FAIL;
    }
    vSemaphoreCreateBinary(led_st.va_led_patttern_sema);
    if (led_st.va_led_patttern_sema == NULL) {
        ESP_LOGE(TAG, "Could not create led pattern semaphore");
        return ESP_FAIL;
    }
    ret = esp_timer_init();
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG, "Could not create esp timer");
    }
    esp_timer_create_args_t va_led_timer_arg = {
        .callback = va_led_delay_timer_cb,
        .arg = NULL,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "ui led timer",
    };
    if (esp_timer_create(&va_led_timer_arg, &led_st.esp_delay_timer_hdl) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create esp timer");
        return ESP_FAIL;
    }
    led_st.va_led_task_handle = xTaskCreateStatic(va_led_task, "ui-led-thread", VA_LED_TASK_STACK_SZ, (void *)va_led_conf,
                                CONFIG_ESP32_PTHREAD_TASK_PRIO_DEFAULT, va_led_task_stack, &va_led_buf);
    if (led_st.va_led_task_handle == NULL) {
        ESP_LOGE(TAG, "Could not create ui led task");
        return ESP_FAIL;
    }

    va_ui_config_t ui_config = {
        .set_state_cb = va_led_set,
        .set_alert_cb = va_led_set_alert,
        .set_dnd_cb = va_led_set_dnd,
    };
    va_ui_init(&ui_config);

    return ESP_OK;
}
