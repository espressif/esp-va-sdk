// Copyright 2018 Espressif Systems (Shanghai) PTE LTD
// All rights reserved.

#include "va_button.h"
#include <media_hal.h>
#include <va_mem_utils.h>
#include "speaker.h"
#include <alerts.h>
#include <tone.h>
#include "va_dsp.h"
#include "va_nvs_utils.h"
#include "va_led.h"


#define VA_BUTTON_QUEUE_LENGTH 1
#define ESP_INTR_FLAG_DEFAULT  0
//#define PA_SPEAKER_EN 33
#define VOLUME_STEP 5
#define VOL_NOTIF_DELAY 850
#define FACTORY_RST_DELAY 10000
#define WIFI_RST_DELAY 5000
#define V_REF   1100

static const char *TAG = "[va_button]";
extern bool ab_but_mute;
static bool va_button_wifi_rst_en = false;
static bool va_button_factory_rst_en = false;
static bool va_button_is_wifi_rst = false;
static bool va_button_is_factory_rst = false;
static va_button_wifi_reset_cb_t va_button_wifi_reset_cb = NULL;
static uint8_t mute_btn_press_flg = 1;

typedef struct {
    bool b_mute;
    esp_adc_cal_characteristics_t characteristics;
    TaskHandle_t va_button_task_handle;
    esp_timer_handle_t esp_timer_handler_f;
    esp_timer_handle_t esp_timer_handler_w;
    xQueueHandle ui_button_queue;
    button_cfg_t but_cfg;
} va_button_t;

static va_button_t button_st = {false, {0}, NULL, NULL, NULL, NULL, {0}};

int number_of_active_alerts = 0;

static esp_err_t va_button_adc_init(uint16_t adc_ch_nm)
{
    esp_err_t ret;
    ret =  adc1_config_width(ADC_WIDTH_BIT_12);
    ret |= adc1_config_channel_atten(adc_ch_nm, ADC_ATTEN_DB_11);
    ret |= esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_12, V_REF, &button_st.characteristics);
    return ret;
}

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
        va_led_set(VA_MUTE_ENABLE);
    }
}

static void va_button_adc_task(void *arg)
{
    //Init ADC and Characteristics
    bool en_timer = 0;
    uint32_t current_adc_val;
    uint8_t current_vol;
    int set_volume = 0;
    int64_t curr_time = esp_timer_get_time();
    bool active_btn_press = false;
    bool act_btn_press_en = false;
    int (*button_exe_internal)(int) = (int(*)(int))arg;
    while (1) {
        esp_adc_cal_get_voltage(button_st.but_cfg.va_button_adc_ch_num, &button_st.characteristics, &current_adc_val);
        //printf("%d: current_adc_val\n", current_adc_val);
        if ((button_st.but_cfg.va_button_adc_val[VA_BUTTON_TAP_TO_TALK] != -1) &&
            (current_adc_val > (button_st.but_cfg.va_button_adc_val[VA_BUTTON_TAP_TO_TALK] - button_st.but_cfg.tolerance)) &&
            (current_adc_val < (button_st.but_cfg.va_button_adc_val[VA_BUTTON_TAP_TO_TALK] + button_st.but_cfg.tolerance))) {    //tap to talk button
            if(button_exe_internal(VA_BUTTON_TAP_TO_TALK)) {
                esp_timer_start_once(button_st.esp_timer_handler_w, (WIFI_RST_DELAY * 1000));
                act_btn_press_en = true;
            }
        } else if ((button_st.but_cfg.va_button_adc_val[VA_BUTTON_MIC_MUTE] != -1) &&
                   (current_adc_val > (button_st.but_cfg.va_button_adc_val[VA_BUTTON_MIC_MUTE] - button_st.but_cfg.tolerance)) &&
                   (current_adc_val < (button_st.but_cfg.va_button_adc_val[VA_BUTTON_MIC_MUTE] + button_st.but_cfg.tolerance))) {    //mic mute button
            if(button_exe_internal(VA_BUTTON_MIC_MUTE)) {
                if(va_boot_is_finish()) {
                    if(mute_btn_press_flg == 1) {
                        button_st.b_mute = true;
                        mute_btn_press_flg = 2;
                    } else if(mute_btn_press_flg == 3) {
                        mute_btn_press_flg = 4;
                        button_st.b_mute = false;
                    }
                }
            }
        } else if ((button_st.but_cfg.va_button_adc_val[VA_BUTTON_VOLUME_UP] != -1) &&
                   (current_adc_val > (button_st.but_cfg.va_button_adc_val[VA_BUTTON_VOLUME_UP] - button_st.but_cfg.tolerance)) &&
                   (current_adc_val < (button_st.but_cfg.va_button_adc_val[VA_BUTTON_VOLUME_UP] + button_st.but_cfg.tolerance))) {    //volume increase button
            if(button_exe_internal(VA_BUTTON_VOLUME_UP)) {
                if(va_boot_is_finish()) {
                    media_hal_get_volume(media_hal_get_handle(), &current_vol);
                    set_volume = current_vol + VOLUME_STEP;
                    if (set_volume > 100) {
                        set_volume = 100;
                        ESP_LOGI(TAG, "volume_max");
                    }
                    media_hal_set_mute(media_hal_get_handle(), 0);
                    volume_to_set = set_volume;
                    va_led_set(VA_SET_VOLUME);
                    media_hal_control_volume(media_hal_get_handle(), set_volume);
                    /* It is good if we play some tone (when nothing else is playing). But there is no dedicated tone for volume. So putting our custom tone. */
                    tone_play(TONE_VOLUME);
                    curr_time = esp_timer_get_time();
                    en_timer = true;
                }
            }
        } else if ((button_st.but_cfg.va_button_adc_val[VA_BUTTON_VOLUME_DOWN] != -1) &&
                   (current_adc_val > (button_st.but_cfg.va_button_adc_val[VA_BUTTON_VOLUME_DOWN] - button_st.but_cfg.tolerance)) &&
                   (current_adc_val < (button_st.but_cfg.va_button_adc_val[VA_BUTTON_VOLUME_DOWN] + button_st.but_cfg.tolerance))) {    //volume decrease button
            if(button_exe_internal(VA_BUTTON_VOLUME_DOWN)) {
                if(va_boot_is_finish()) {
                    media_hal_get_volume(media_hal_get_handle(), &current_vol);
                    set_volume = current_vol - VOLUME_STEP;
                    if (set_volume < 0) {
                        set_volume = 0;
                        ESP_LOGI(TAG, "volume_min");
                    }
                    media_hal_set_mute(media_hal_get_handle(), 0);
                    volume_to_set = set_volume;
                    va_led_set(VA_SET_VOLUME);
                    media_hal_control_volume(media_hal_get_handle(), set_volume);
                    /* It is good if we play some tone (when nothing else is playing). But there is no dedicated tone for volume. So putting our custom tone. */
                    tone_play(TONE_VOLUME);
                    curr_time = esp_timer_get_time();
                    en_timer = true;
                }
            }
        } else if ((button_st.but_cfg.va_button_adc_val[VA_BUTTON_FACTORY_RST] != -1) &&
                   (current_adc_val > (button_st.but_cfg.va_button_adc_val[VA_BUTTON_FACTORY_RST] - button_st.but_cfg.tolerance)) &&
                   (current_adc_val < (button_st.but_cfg.va_button_adc_val[VA_BUTTON_FACTORY_RST] + button_st.but_cfg.tolerance))) {    //factory reset case
            if(button_exe_internal(VA_BUTTON_FACTORY_RST)) {
                esp_timer_start_once(button_st.esp_timer_handler_f, (FACTORY_RST_DELAY * 1000));
            }
        } else if((button_st.but_cfg.va_button_adc_val[VA_BUTTON_CUSTOM_1] != -1) &&
                  (current_adc_val > (button_st.but_cfg.va_button_adc_val[VA_BUTTON_CUSTOM_1] - button_st.but_cfg.tolerance)) &&
                  (current_adc_val < (button_st.but_cfg.va_button_adc_val[VA_BUTTON_CUSTOM_1] + button_st.but_cfg.tolerance))) {    //button number 5 - custom
            if(button_exe_internal(VA_BUTTON_CUSTOM_1)) {
            }
        } else if((button_st.but_cfg.va_button_adc_val[VA_BUTTON_CUSTOM_2] != -1) &&
                  (current_adc_val > (button_st.but_cfg.va_button_adc_val[VA_BUTTON_CUSTOM_2] - button_st.but_cfg.tolerance)) && 
                  (current_adc_val < (button_st.but_cfg.va_button_adc_val[VA_BUTTON_CUSTOM_2] + button_st.but_cfg.tolerance))) {    //button number 6 - custom
            if(button_exe_internal(VA_BUTTON_CUSTOM_2)) {
            }
        } else if (current_adc_val > button_st.but_cfg.va_button_adc_val[VA_BUTTON_VAL_IDLE]) {    //idle case
            if(button_exe_internal(VA_BUTTON_VAL_IDLE)) {
                esp_timer_stop(button_st.esp_timer_handler_w);  //Stop wifi reset timer
                esp_timer_stop(button_st.esp_timer_handler_f);  //Stop factory reset timer
                if(act_btn_press_en) {
                    active_btn_press = true;
                }
                if(mute_btn_press_flg == 2) {
                    va_dsp_mic_mute(button_st.b_mute);
                    va_led_set(VA_MUTE_ENABLE);
                    tone_play(TONE_PRIVACY_ON);
                    mute_btn_press_flg = 3;
                } else if (mute_btn_press_flg == 4) {
                    va_dsp_mic_mute(button_st.b_mute);
                    va_led_set(VA_MUTE_DISABLE);
                    tone_play(TONE_PRIVACY_OFF);
                    mute_btn_press_flg = 1;
                }
            }
        }
        if (((esp_timer_get_time() - curr_time) > (VOL_NOTIF_DELAY * 1000)) && en_timer) {
            speaker_notify_vol_changed(set_volume);
            va_led_set(VA_SET_VOLUME_DONE);
            en_timer = false;
        }
        if(va_button_wifi_rst_en) {
            if (va_button_wifi_reset_cb == NULL) {
                ESP_LOGE(TAG, "No callback set for wifi reset");
            } else {
                /* Do not set any led here. Instead set the Factory reset led when starting provisioning after restart. */
                va_led_set(LED_OFF);
                va_reset();
                (*va_button_wifi_reset_cb)(NULL);
                va_button_is_wifi_rst = true;   //Since non-blocking as of now
            }
            va_button_wifi_rst_en = false;
        }
        if(va_button_factory_rst_en) {
            va_button_is_factory_rst = true;   //Lets keep for it, can be removed as this is a blocking call
            /* Do not set any led here. Instead set the Factory reset led when starting provisioning after restart. */
            va_led_set(LED_OFF);
            va_nvs_flash_erase();
            va_reset();
            esp_restart();
            va_button_factory_rst_en = false;
        }
        if(active_btn_press && act_btn_press_en && !(va_button_factory_rst_en) && !(va_button_is_wifi_rst)) {
            printf("%s: Tap to talk button pressed\n", TAG);
            if (number_of_active_alerts > 0) {
#ifdef VOICE_ASSISTANT_ALEXA
                if (alerts_is_active() == true) {
                    alerts_stop_currently_active();
                }
#endif /* VOICE_ASSISTANT_ALEXA */
            } else {
                if (((ab_but_mute) || !(button_st.b_mute)) && (va_boot_is_finish())) {
                    va_dsp_tap_to_talk_start();
                }
            }
            active_btn_press = false;
            act_btn_press_en = false;
        }
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}


static void IRAM_ATTR gpio_isr_handler_t(void *arg)
{
    uint32_t gpio_num = (uint32_t) arg;
    xQueueSendFromISR(button_st.ui_button_queue, &gpio_num, NULL);
}

static esp_err_t va_button_gpio_init()
{
    gpio_config_t io_conf;
    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
    for(int i = 0; i < (VA_BUTTON_MAX - 1); i++) {                // Since gpio is interrupt based, Idle is not required. Hence (MAX -1)
        if(button_st.but_cfg.va_button_gpio_num[i] != -1) {
            if(i == VA_BUTTON_MIC_MUTE) {
                io_conf.intr_type = GPIO_INTR_NEGEDGE;
                io_conf.pin_bit_mask = ((((uint64_t) 1) << button_st.but_cfg.va_button_gpio_num[i]));
                io_conf.mode = GPIO_MODE_INPUT;
                gpio_config(&io_conf);
                gpio_isr_handler_add(button_st.but_cfg.va_button_gpio_num[i], gpio_isr_handler_t, (void *) button_st.but_cfg.va_button_gpio_num[i]);
            } else {
                io_conf.intr_type = GPIO_INTR_POSEDGE;
                io_conf.pin_bit_mask = ((((uint64_t) 1) << button_st.but_cfg.va_button_gpio_num[i]));
                io_conf.mode = GPIO_MODE_INPUT;
                io_conf.pull_up_en = 1;
                gpio_config(&io_conf);
                gpio_isr_handler_add(button_st.but_cfg.va_button_gpio_num[i], gpio_isr_handler_t, (void *) button_st.but_cfg.va_button_gpio_num[i]);
            }
        } 
    }
    return ESP_OK;
}

static void va_button_gpio_task(void *arg)
{
    int io_num;
    uint8_t mute_btn_press_en = false;
    while (1) {
        if (xQueueReceive(button_st.ui_button_queue, &io_num, 0) != pdTRUE) {
            //ESP_LOGE(TAG, "Failed to receive from led queue");
            vTaskDelay(100/portTICK_RATE_MS);
        } else {
            printf("%s: button pressed: %d\n", TAG, io_num);
            if (io_num == button_st.but_cfg.va_button_gpio_num[VA_BUTTON_TAP_TO_TALK]) {
                ESP_LOGI(TAG, "tap to talk pressed");
                va_dsp_tap_to_talk_start();
                vTaskDelay(pdMS_TO_TICKS(333));
            } else if (io_num == button_st.but_cfg.va_button_gpio_num[VA_BUTTON_MIC_MUTE]) {
                esp_timer_start_once(button_st.esp_timer_handler_f, (FACTORY_RST_DELAY * 1000));
                mute_btn_press_en = true;
            }
        }
        if(mute_btn_press_en && gpio_get_level(button_st.but_cfg.va_button_gpio_num[VA_BUTTON_MIC_MUTE]) == 1) {
            esp_timer_stop(button_st.esp_timer_handler_f);
            mute_btn_press_en = false;
        }
        if(va_button_factory_rst_en) {
            /* Do not set any led here. Instead set the Factory reset led when starting provisioning after restart. */
            va_led_set(LED_OFF);
            vTaskDelay(1000 / portTICK_RATE_MS);
            va_nvs_flash_erase();
            va_dsp_reset();
            media_hal_deinit(media_hal_get_handle());
            esp_restart();
        }

    }
}

void va_button_register_wifi_reset_cb(va_button_wifi_reset_cb_t wifi_reset_cb)
{
    va_button_wifi_reset_cb = wifi_reset_cb;
}

esp_err_t va_button_init(const button_cfg_t *button_cfg, int (*button_event_cb)(int))
{
    button_st.but_cfg = *button_cfg;

    esp_timer_init();
    esp_timer_create_args_t timer_arg = {
        .callback = va_button_factory_reset_timer_cb,
        .arg = NULL,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "va button factory reset timer",
    };
    if (esp_timer_create(&timer_arg, &button_st.esp_timer_handler_f) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create esp timer for reset-to-factory button");
        return ESP_FAIL;
    }
    timer_arg.callback = va_button_wifi_reset_timer_cb;
    timer_arg.name = "va button wifi reset timer";
    if (esp_timer_create(&timer_arg, &button_st.esp_timer_handler_w) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create esp timer for reset-to-wifi button");
        return ESP_FAIL;
    }

    if(button_st.but_cfg.is_adc) {
        va_button_adc_init(button_st.but_cfg.va_button_adc_ch_num);
        if(button_st.but_cfg.va_button_adc_val[VA_BUTTON_MIC_MUTE] == -1) {
            button_st.b_mute = true;
        }

        StackType_t *va_button_task_stack = (StackType_t *)va_mem_alloc(VA_BUTTON_TASK_BUFFER_SZ, VA_MEM_EXTERNAL);
        static StaticTask_t va_button_task_buf;
        button_st.va_button_task_handle = xTaskCreateStatic(va_button_adc_task, "va-button-thread", VA_BUTTON_TASK_BUFFER_SZ,
                                          (void *)button_event_cb, CONFIG_ESP32_PTHREAD_TASK_PRIO_DEFAULT, va_button_task_stack, &va_button_task_buf);
        if (button_st.va_button_task_handle == NULL) {
            ESP_LOGE(TAG, "Could not create va button task");
            return ESP_FAIL;
        }
    } else if(button_st.but_cfg.is_gpio) {
        button_st.ui_button_queue = xQueueCreate(VA_BUTTON_QUEUE_LENGTH, sizeof(uint32_t));
        if (button_st.ui_button_queue == NULL) {
            ESP_LOGE(TAG, "Could not create queue");
            return ESP_FAIL;
        }
        va_button_gpio_init();
        StackType_t *va_button_task_stack = (StackType_t *)va_mem_alloc(VA_BUTTON_TASK_BUFFER_SZ, VA_MEM_EXTERNAL);
        static StaticTask_t va_button_task_buf;
        button_st.va_button_task_handle = xTaskCreateStatic(va_button_gpio_task, "va-button-thread", VA_BUTTON_TASK_BUFFER_SZ,
                                          NULL, CONFIG_ESP32_PTHREAD_TASK_PRIO_DEFAULT, va_button_task_stack, &va_button_task_buf);
        if (button_st.va_button_task_handle == NULL) {
            ESP_LOGE(TAG, "Could not create va button task");
            return ESP_FAIL;
        }
    }
    return ESP_OK;
}
