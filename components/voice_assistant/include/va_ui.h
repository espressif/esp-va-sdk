/*
 * ESPRESSIF MIT License
 *
 * Copyright (c) 2018 <ESPRESSIF SYSTEMS (SHANGHAI) PTE LTD>
 *
 * Permission is hereby granted for use on all ESPRESSIF SYSTEMS products, in which case,
 * it is free of charge, to any person obtaining a copy of this software and associated
 * documentation files (the "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the Software is furnished
 * to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */
#pragma once

#include <stdbool.h>
#include <esp_err.h>
#include <voice_assistant_app_cb.h>

/* Configuration to be passed to the `va_ui_init` */
typedef struct va_ui_config {
    /* Callback to set_state */
    void (*set_state_cb)(int va_state);
    /* Callback to set_alert */
    void (*set_alert_cb)(alexa_alert_types_t alert_type, alexa_alert_state_t alert_state);
    /* Callback to set_dnd */
    void (*set_dnd_cb)(bool dnd_state);
} va_ui_config_t;

extern uint8_t volume_to_set;

/* User Interface Initialisation
 *
 * This API registers the callbacks for the other APIs.
 *
 * \param[in] ui_config     Pointer to `va_ui_config_t`. For example, you can set the callbacks for `va_led` APIs.
 */
void va_ui_init(va_ui_config_t *ui_config);

/* User Interface Set State
 *
 * This API calls the `set_state_cb` from `va_ui_config_t`.
 *
 * \param[in] va_state      This can be one of the states from `voice_assistant_app_cb.h`.
 */
void va_ui_set_state(int va_state);

/* User Interface Set Alert
 *
 * This API calls the `set_alert_cb` from `va_ui_config_t`.
 *
 * \param[in] alert_type    alexa_alert_types_t from `voice_assistant_app_cb.h`.
 * \param[in] alert_state   alexa_alert_state_t from `voice_assistant_app_cb.h`.
 */
void va_ui_set_alert(alexa_alert_types_t alert_type, alexa_alert_state_t alert_state);

/* User Interface Set Do Not Disturb
 *
 * This API calls the `set_dnd_cb` from `va_ui_config_t`.
 *
 * \param[in] dnd_state     true if DND is active, otherwise false.
 */
void va_ui_set_dnd(bool dnd_state);
