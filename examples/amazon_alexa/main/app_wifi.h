// Copyright 2018 Espressif Systems (Shanghai) PTE LTD
// All rights reserved.

#pragma once

int8_t app_wifi_get_reset_to_prov();
void app_wifi_unset_reset_to_prov();
int app_wifi_reset_to_prov_init();
void app_wifi_start_timeout_timer();
void app_wifi_stop_timeout_timer();

