// Copyright 2018 Espressif Systems (Shanghai) PTE LTD
// All rights reserved.

#ifndef _VOICE_ASSISTANT_H_
#define _VOICE_ASSISTANT_H_

#include <stdlib.h>
#include <stdbool.h>

/** Get current Voice Assistant SDK version
 *
 * \return Pointer to version string
 */
const char *va_get_sdk_version();

/**
 * Boot initialization
 */
void va_boot_init();

/**
 * Wait for dsp to signal finishing of dsp_init
 */
void va_boot_dsp_wait();

/**
 * Signal finish of dsp_init
 */
void va_boot_dsp_signal();

/**
 * API to check if boot is finished or not
 */
bool va_boot_is_finish();

typedef void (*va_signout_handler_t)(void);

/**
 * API to disable the buttons (except factory reset and wifi reset)
 */
void va_button_suspend();

/**
 * API to enable the buttons
 * This needs to only be called if va_button_suspend() has been called earlier and the functionality of the buttons needs to be added again.
 */
void va_button_resume();

/**
 * API to reset the Voice Assistant.
 * This can be called before rebooting the device.
 */
void va_reset();

#endif /*_VOICE_ASSISTANT_H_ */
