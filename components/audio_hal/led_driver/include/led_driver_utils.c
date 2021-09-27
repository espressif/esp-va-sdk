// Copyright 2018 Espressif Systems (Shanghai) PTE LTD
// All rights reserved.

#include <led_driver.h>

/*
In the structure, the order is always RGB, i.e. first integer is always the position of RED, second is position of GREEN, third is position of BLUE.
Example,
- for the order R_G_B, the array would be {1,2,3}.
- for the order B_G_R, the array would be {3,2,1}. (red == third, green == second, blue == first)
- for the order G_B_R, the array would be {3,1,2}. (red == third, green == first, blue == second)
*/
struct led_color_offset led_color_offset[LED_ORDER_MAX_PATTERNS] = {
    [LED_ORDER_RED_GREEN_BLUE] = {1, 2, 3},
    [LED_ORDER_RED_BLUE_GREEN] = {1, 3, 2},
    [LED_ORDER_BLUE_RED_GREEN] = {2, 3, 1},
    [LED_ORDER_GREEN_RED_BLUE] = {2, 1, 3},
    [LED_ORDER_GREEN_BLUE_RED] = {3, 1, 2},
    [LED_ORDER_BLUE_GREEN_RED] = {3, 2, 1},
};
