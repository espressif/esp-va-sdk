// Copyright 2019 Espressif Systems (Shanghai) PTE LTD
// All rights reserved.

#pragma once
#include <alexa.h>

esp_err_t alexa_local_config_start(alexa_config_t *va_cfg, const char *service_name);
esp_err_t alexa_local_config_stop();
esp_err_t alexa_local_config_set_name(const char *new_name);
const char *alexa_local_config_get_name();

/**
 * This returns a pointer to protocomm_t object. This can be used to extend/add new application specific
 * HTTP handlers.
 * Application can call protocomm_add_endpoint() and pass this pointer as parameter to the function to
 * create a new application specific endpoint with custom handler.
 *
 * @return protocomm_t* Pointer to protocomm_t object.
 * */
protocomm_t *alexa_local_config_get_endpoint();
