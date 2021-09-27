// Copyright 2018 Espressif Systems (Shanghai) PTE LTD
// All rights reserved.

#pragma once

#include <stdint.h>
#include "esp_a2dp_api.h"
#include "esp_avrc_api.h"
#include "abstract_rb.h"
#include "bluetooth.h"

/**
 * @brief     callback function for A2DP sink
 */
void bt_app_a2d_sink_cb(esp_a2d_cb_event_t event, esp_a2d_cb_param_t *param);

/**
 * @brief     callback function for A2DP source
 */
void bt_app_a2d_source_cb(esp_a2d_cb_event_t event, esp_a2d_cb_param_t *param);

/**
 * @brief     callback function for A2DP sink audio data stream
 */
void bt_app_a2d_data_cb(const uint8_t *data, uint32_t len);

/**
 * @brief     callback function for AVRCP controller
 */
void bt_app_rc_ct_cb(esp_avrc_ct_cb_event_t event, esp_avrc_ct_cb_param_t *param);
void bt_app_rc_tg_cb(esp_avrc_tg_cb_event_t event, esp_avrc_tg_cb_param_t *param);

int bt_av_init(bt_event_handler_t event_cb);

rb_handle_t bt_app_av_get_rb();

//int bt_av_source_play(const uint8_t *data, size_t len);
void bt_app_stop_media();

void bt_app_av_notify_vol_change(int volume);