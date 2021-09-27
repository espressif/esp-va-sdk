/*
*
* Copyright 2015-2018 Espressif Systems (Shanghai) PTE LTD
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*
*/

#pragma once

#include <media_hal.h>

/**
 * @brief Set media_hal functions audio_codec fuctions and do startup initializations.
 *
 *        - Audio codec package must implement this function and other functions to be set in `media_hal_t`.
 *        - Please refer to `media_hal_t` for details on functions to be assigned.
 */
esp_err_t media_hal_codec_init(media_hal_t *media_hal, media_hal_config_t *media_hal_conf);
