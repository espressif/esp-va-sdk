// Copyright 2018 Espressif Systems (Shanghai) PTE LTD
// All rights reserved.

#include <string.h>
#include <esp_log.h>

#include <core2forAWS.h>
#include <button_driver.h>

#define TAG "button_driver_m5_core2_aws"

#define BUTTON_LEFT     (((int64_t)1) << 0)
#define BUTTON_MIDDLE   (((int64_t)1) << 1)
#define BUTTON_RIGHT    (((int64_t)1) << 2)

static bool is_init_done = false;
static bool enable_debug = false;
static int current_button_value = 0;
static button_driver_config_t button_config;

static esp_err_t button_driver_m5_core2_aws_init()
{
    /* Initialize the driver */
	Core2ForAWS_Button_Init();
    return ESP_OK;
}

static int button_driver_get_current_value()
{
    int current_value = 0;
    /* Get the current button_value */
    if (Button_IsPress(button_left)) {
        current_value |= BUTTON_LEFT;
    }
    if (Button_IsPress(button_middle)) {
        current_value |= BUTTON_MIDDLE;
    }
    if (Button_IsPress(button_right)) {
        current_value |= BUTTON_RIGHT;
    }

    return current_value;
}

static bool button_driver_check_value(button_event_t event)
{
    /* Check if the current_button_value matches any event */
    if (button_config.button_val[event] == -1 || current_button_value == 0) {
        /* Button value for event is not set */
        return false;
    }
    if (current_button_value == (current_button_value & button_config.button_val[event])) {
        /* Current button value matches the event. This also checks for combination of buttons. */
        return true;
    }

    /* Button value for this event is not set */
    return true;
}

void button_driver_enable_debug(bool enable)
{
    enable_debug = enable;
}

button_event_t button_driver_get_event()
{
    /* Get the current button value */
    current_button_value = button_driver_get_current_value();

    if (enable_debug) {
        printf("%s: Current button value: %d\n", TAG, current_button_value);
    }

    /* Checking for each button_event */
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

    /* Copy the button configuration */
    memcpy(&button_config, button_driver_config, sizeof(button_driver_config_t));

    /* M5_Core2_AWS specific Buttons driver is already initalized in Core2ForAWS_Init() */

    is_init_done = true;
    return ESP_OK;
}
