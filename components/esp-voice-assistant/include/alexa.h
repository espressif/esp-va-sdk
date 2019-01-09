// Copyright 2018 Espressif Systems (Shanghai) PTE LTD
// All rights reserved.

#ifndef _ALEXA_H_
#define _ALEXA_H_

#include "voice_assistant.h"

/** The Alexa Configuration Structure
 */
typedef struct {
    /** Configuration for the Auth Delegate */
    auth_delegate_config_t auth_delegate;
    va_playback_config_t va_playback;
} alexa_config_t;

/** Initialize Alexa
 *
 * This call must be made after the Wi-Fi connection has been established with the configured Access Point.
 *
 * \param[in] cfg The Alexa Configuration
 *
 * \return
 *    - 0 on Success
 *    - an error code otherwise
 */
int alexa_init(alexa_config_t *cfg);
#endif /*_ALEXA_H_ */
