// Copyright 2018 Espressif Systems (Shanghai) PTE LTD
// All rights reserved.

#include <string.h>
#include <stdint.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <driver/gpio.h>

#include <button_driver.h>

#define ESP_INTR_FLAG_DEFAULT  0
static const char *TAG = "button_driver_gpio";

static bool is_init_done = false;
static bool enable_debug = false;
static uint64_t all_gpio_list = 0;
static uint64_t current_gpio_list = 0;
xQueueHandle button_driver_queue = NULL;
static button_driver_config_t button_config;

static void IRAM_ATTR button_driver_gpio_isr_handler(void *arg)
{
    /* This interrupt handler is called when any button is pressed. */
    uint32_t gpio_num = (uint32_t)arg;
    xQueueSendFromISR(button_driver_queue, &gpio_num, NULL);
}

static void button_driver_wait_for_button_press()
{
    uint32_t gpio_num;
    /* Wait here until there is something in the queue to receive. Enqueue to this queue will be done by the gpio_isr_haldler(). */
    xQueueReceive(button_driver_queue, &gpio_num, portMAX_DELAY);
    /* The current gpio which triggered the interrupt is received from the queue. It is not being used now, but can be used later.
    For events with combinations of gpio, only one gpio (least significant) is sent/received. This is okay since it is just used to unblock from queue_receive. */
}

static esp_err_t button_driver_gpio_init()
{
    esp_err_t ret = ESP_OK;
    gpio_config_t gpio_cfg;
    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
    for(button_event_t event = 0; event < BUTTON_EVENT_MAX; event++) {
        if (button_config.button_val[event] != -1) {
            all_gpio_list |= button_config.button_val[event];

            gpio_cfg.intr_type = GPIO_INTR_LOW_LEVEL;
            gpio_cfg.pin_bit_mask = ((int64_t)1) << (ffsll(button_config.button_val[event]) - 1);
            gpio_cfg.mode = GPIO_MODE_INPUT;
            gpio_cfg.pull_up_en = 1;

            gpio_config(&gpio_cfg);
            gpio_isr_handler_add(ffsll(button_config.button_val[event]) - 1, button_driver_gpio_isr_handler, (void *)(ffsll(button_config.button_val[event]) - 1));
        }
    }
    return ret;
}

static uint64_t button_driver_get_all_gpio()
{
    /* Just checking for gpios which are a part of any event. For events with combination of gpios, all the gpios are checked. */
    uint64_t all_gpio = 0;
    int64_t button_val = all_gpio_list;
    while (button_val != 0) {
        int32_t next_gpio = ffsll(button_val) - 1;
        int64_t next_gpio_bit = ((int64_t)1) << next_gpio;
        if (gpio_get_level(next_gpio) == 0) {
            all_gpio |= next_gpio_bit;
        }
        button_val = button_val & (~next_gpio_bit);
    }
    return all_gpio;
}

static bool button_driver_check_value(button_event_t event)
{
    if (button_config.button_val[event] == -1 || current_gpio_list == 0) {
        /* GPIO value for event is not set */
        return false;
    }
    if (current_gpio_list == (current_gpio_list & button_config.button_val[event])) {
        /* GPIO value is set in the gpio_list. This also checks for combination of gpios. */
        return true;
    }

    /* GPIO value for this event is not set */
    return false;
}

void button_driver_enable_debug(bool enable)
{
    enable_debug = enable;
}

button_event_t button_driver_get_event()
{
    static bool wait_for_button_press = false;
    if (wait_for_button_press) {
        /* This is a blocking call. It is unblocked when the interrupt is triggered on button press */
        button_driver_wait_for_button_press();
    }

    current_gpio_list = button_driver_get_all_gpio();

    if (enable_debug) {
        printf("%s: Current GPIO list: %lld\n", TAG, current_gpio_list);
    }

    /* Don't wait for button press in the next call, if the previous event is not idle. If this is not done, idle would never be returned. */
    wait_for_button_press = false;

    if (button_driver_check_value(BUTTON_EVENT_IDLE)) {
        wait_for_button_press = true;
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
    wait_for_button_press = true;
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

#define QUEUE_LENGTH 1
    button_driver_queue = xQueueCreate(QUEUE_LENGTH, sizeof(uint32_t));
    if (button_driver_queue == NULL) {
        ESP_LOGE(TAG, "Could not create queue");
        return ESP_FAIL;
    }

    button_driver_gpio_init();

    is_init_done = true;
    return ESP_OK;
}
