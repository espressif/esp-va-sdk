// Copyright 2018 Espressif Systems (Shanghai) PTE LTD
// All rights reserved.

#ifndef _LED_DRIVER_H_
#define _LED_DRIVER_H_

#include <stdint.h>
#include <stdbool.h>
#include <esp_err.h>

/** Specifies the order in which 0xFFFFFF is represented for the LEDs */
typedef enum led_color_order {
    LED_ORDER_RED_GREEN_BLUE,
    LED_ORDER_RED_BLUE_GREEN,
    LED_ORDER_BLUE_GREEN_RED,
    LED_ORDER_BLUE_RED_GREEN,
    LED_ORDER_GREEN_BLUE_RED,
    LED_ORDER_GREEN_RED_BLUE,
    LED_ORDER_MAX_PATTERNS,
} led_color_order_t;

/** Specifies the type of the LED driver */
typedef enum led_driver_type {
    LED_DRIVER_TYPE_SINGLE,
    LED_DRIVER_TYPE_ARRAY,
} led_driver_type_t;

/** LED driver configuration */
typedef struct {
    led_driver_type_t type;
    led_color_order_t led_order;

    /** Used for LED_DRIVER_TYPE_SINGLE */
    /** Set to -1 if not being used */
    int red_gpio;
    /** Set to -1 if not being used */
    int green_gpio;
    /** Set to -1 if not being used */
    int blue_gpio;

    /** Used for LED_DRIVER_TYPE_ARRAY */
    /** Total number of LEDs */
    int num_of_leds;
    /** Starting GPIO pin for the LED array */
    int start_gpio_pin;
    /** LEDs in a set, for multiple sets of LEDs.
    Example: If there are 2 rows of 5 LEDs with both rows displaying the same led_pattern, then set num_of_leds to 10 and leds_in_set to 5. */
    int leds_in_set;
} led_driver_config_t;

/** Initialization check
 *
 * This API checks if LED driver has been initialized.
 *
 * @return true if initialization is done.
 * @return false if initialization is not done.
 */
bool led_driver_is_init_done();

/** Initialize LED driver
 *
 * Initialize the LED driver selected by the LED_DRIVER_PATH.
 *
 * @param[in] led_driver_config Configuration of the LEDs on the board.
 *
 * @return ESP_OK on success.
 * @return error in case of failure.
 */
esp_err_t led_driver_init(led_driver_config_t *led_driver_config);

/** Set LED value
 *
 * This API sets the value of RGB for the LEDs.
 *
 * @param[in] led_value Array of integers where each integer represents the RGB value for one LED.
 */
void led_driver_set_value(const uint32_t *led_value);

/* Used internally */
struct led_color_offset {
    int red;
    int green;
    int blue;
};
extern struct led_color_offset led_color_offset[LED_ORDER_MAX_PATTERNS];

#endif /* _LED_DRIVER_H_ */
