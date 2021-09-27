// Copyright 2018 Espressif Systems (Shanghai) PTE LTD
// All rights reserved.

#pragma once

#include <esp_log.h>

/**
 * @brief Register speech started callback
 *
 * @param callback speech started callback function to register
 *
 * @return     int, 0--success, others--fail
 */
esp_err_t speech_synthesizer_register_speech_started_cb(int (*callback) (void));

/**
 * @brief Deregister speech started callback
 *
 * @param callback Speech started callback function to deregister
 *
 * @return     int, 0--success, others--fail
 */
esp_err_t speech_synthesizer_deregister_speech_started_cb(int (*callback) (void));
