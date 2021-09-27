// Copyright 2018 Espressif Systems (Shanghai) PTE LTD
// All rights reserved.

#pragma once

#include <esp_err.h>

enum prompt_type {
    PROMPT_HELLO = 0,
    PROMPT_SETUP_MODE = 1,
    PROMPT_SETUP_MODE_ON = 2,
    PROMPT_SETUP_MODE_OFF = 3,
    PROMPT_WIFI_PASSWORD_INCORRECT = 4,
    PROMPT_FACTORY_DATA_RESET = 5,
    PROMPT_TROUBLE_CONNECTING_TO_INTERNET = 6,
    PROMPT_TROUBLE_UNDERSTANDING = 7,
    PROMPT_LOST_CONNECTION = 8,
    PROMPT_DEVICE_READY = 9,
    PROMPT_MAX = 30,
};

struct prompt {
    const uint8_t *start;
    const uint8_t *end;
};

int prompt_init(struct prompt va_prompts[PROMPT_MAX]);
esp_err_t prompt_play(enum prompt_type type);
esp_err_t prompt_set_custom(enum prompt_type type, const uint8_t *start, const uint8_t *end);
