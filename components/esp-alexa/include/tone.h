// Copyright 2018 Espressif Systems (Shanghai) PTE LTD
// All rights reserved.

#pragma once

#include <stdint.h>

typedef enum {
    TONE_WAKE,
    TONE_WAKE_TOUCH,
    TONE_ENDPOINT,
    TONE_PRIVACY_ON,
    TONE_PRIVACY_OFF,
    TONE_ALARM_SHORT,
    TONE_TIMER_SHORT,
    TONE_MAX,
} tone_type_t;

int tone_play(tone_type_t type);
/** This API will enable a tone which would be played if dialog is in progress and timer/alarm goes off.
 * Enabling this tone would increase size of binary by around 768 KB */
void tone_enable_larger_tones();
