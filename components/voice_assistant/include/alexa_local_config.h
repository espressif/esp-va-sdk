// Copyright 2019 Espressif Systems (Shanghai) PTE LTD
// All rights reserved.

#pragma once
#include <alexa.h>

esp_err_t alexa_local_config_start(alexa_config_t *va_cfg, char *service_name);
esp_err_t alexa_local_config_stop();
