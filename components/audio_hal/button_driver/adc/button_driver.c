// Copyright 2018 Espressif Systems (Shanghai) PTE LTD
// All rights reserved.

#include <string.h>
#include <esp_log.h>
#include <esp_adc_cal.h>
#include <driver/adc.h>

#include <button_driver.h>

#define V_REF   1100
static const char *TAG = "button_driver_adc";

static bool is_init_done = false;
static esp_adc_cal_characteristics_t characteristics;
static bool enable_debug = false;
static uint32_t current_adc_val;
static button_driver_config_t button_config;

static esp_err_t button_driver_adc_init()
{
    esp_err_t ret;
    ret =  adc1_config_width(ADC_WIDTH_BIT_12);
    ret |= adc1_config_channel_atten(button_config.ch_num, ADC_ATTEN_DB_11);
    ret = esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_12, V_REF, &characteristics);
    return ret;
}

static bool button_driver_check_value(button_event_t event)
{
    if (button_config.button_val[event] == -1) {
        /* ADC value for event is not set */
        return false;
    }
    if (current_adc_val < (button_config.button_val[event] - button_config.tolerance)) {
        /* Current ADC value is less than the event */
        return false;
    }
    if (current_adc_val > (button_config.button_val[event] + button_config.tolerance)) {
        /* Current ADC value is more than the event */
        return false;
    }

    /* Current ADC value matches the event */
    return true;
}

void button_driver_enable_debug(bool enable)
{
    enable_debug = enable;
}

button_event_t button_driver_get_event()
{
    esp_adc_cal_get_voltage(button_config.ch_num, &characteristics, &current_adc_val);

    if (enable_debug) {
        printf("%s: Current ADC value: %d\n", TAG, current_adc_val);
    }

    if (button_driver_check_value(BUTTON_EVENT_IDLE)) {
        return BUTTON_EVENT_IDLE;
    } else if (button_driver_check_value(BUTTON_EVENT_TAP_TO_TALK)) {
        return BUTTON_EVENT_TAP_TO_TALK;
    } else if (button_driver_check_value(BUTTON_EVENT_MIC_MUTE)) {
        return BUTTON_EVENT_MIC_MUTE;
    } else if (button_driver_check_value(BUTTON_EVENT_VOLUME_UP)) {
        return BUTTON_EVENT_VOLUME_UP;
    } else if (button_driver_check_value(BUTTON_EVENT_VOLUME_DOWN)) {
        return BUTTON_EVENT_VOLUME_DOWN;
    } else if (button_driver_check_value(BUTTON_EVENT_FACTORY_RST)) {
        return BUTTON_EVENT_FACTORY_RST;
    } else if (button_driver_check_value(BUTTON_EVENT_CUSTOM_1)) {
        return BUTTON_EVENT_CUSTOM_1;
    } else if (button_driver_check_value(BUTTON_EVENT_CUSTOM_2)) {
        return BUTTON_EVENT_CUSTOM_2;
    }

    /* Returning IDLE */
    return BUTTON_EVENT_IDLE;
}

bool button_driver_is_init_done()
{
    return is_init_done;
}

esp_err_t button_driver_init(button_driver_config_t *button_driver_config)
{
    ESP_LOGI(TAG, "Initialising button driver");
    
    memcpy(&button_config, button_driver_config, sizeof(button_driver_config_t));
    
    button_driver_adc_init();

    is_init_done = true;
    return ESP_OK;
}
