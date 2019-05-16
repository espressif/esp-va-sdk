// Copyright 2018 Espressif Systems (Shanghai) PTE LTD
// All rights reserved.

#include <string.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>
#include <str_utils.h>
#include <equalizer_controller.h>

#include "equalizer.h"

static const char *TAG = "[equalizer]";

void equalizer_change_band_level(int bass, int midrange, int treble)
{
    ESP_LOGI(TAG, "Setting base: %d, midrange: %d, treble: %d", bass, midrange, treble);
}

void equalizer_set_mode(enum equalizer_controller_mode mode, int *bass, int *midrange, int *treble)
{
    /* MAke sure these values are within the limits. */
    switch (mode) {
        case EQUALIZER_CONTROLLER_MOVIE:
            *bass = 2;
            *midrange = 2;
            *treble = 2;
            break;

        case EQUALIZER_CONTROLLER_NIGHT:
            *bass = 1;
            *midrange = 1;
            *treble = 1;
            break;

        default:
            *bass = 0;
            *midrange = 0;
            *treble = 0;
            break;
    }
    equalizer_change_band_level(*bass, *midrange, *treble);
}

int equalizer_init()
{
    int bass, midrange, treble;
    enum equalizer_controller_mode current_mode = EQUALIZER_CONTROLLER_MOVIE;
    equalizer_set_mode(current_mode, &bass, &midrange, &treble);

    equalizer_controller_init(current_mode, bass, midrange, treble, EQUALIZER_BAND_LEVEL_MIN, EQUALIZER_BAND_LEVEL_MAX, equalizer_set_mode, equalizer_change_band_level);

    // If the equalizer is changed using the on-device buttons.
    equalizer_controller_notify_level_change(EQUALIZER_CONTROLLER_NIGHT, 1, 1, 1);

    return 0;
}