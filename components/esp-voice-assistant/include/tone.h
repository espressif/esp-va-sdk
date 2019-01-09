// Copyright 2018 Espressif Systems (Shanghai) PTE LTD
// All rights reserved.

#pragma once

#include <stdint.h>
#include <voice_assistant_app_cb.h>

typedef enum {
    TONE_WAKE,
    TONE_WAKE_TOUCH,
    TONE_ENDPOINT,
    TONE_PRIVACY_ON,
    TONE_PRIVACY_OFF,
    TONE_ALARM_SHORT,
    TONE_TIMER_SHORT,
    TONE_MAX,
} tone_type_t;

/** This API will play custom tones provided by the application (should be the part of firmware itself)
 *
 * Please check "Build System" section of ESP-IDF Programming guide to know how to make changes in Makefile or component.mk to embed binary data in firmware and get start and end pointers of the binary.
 * Supported files types: wav, mp3
 *
 * \param[in] start Pointer to start of tone in flash
 * \param[in] end Pointer to end of tone in flash
 * \param[in] playback_param Pointer to playback parameters. Please note this needs to point to valid location while the tone is being played. It's recommeded that it is declared as either global or static variable.
 * \return 0 on success, -1 otherwise
 */
int tone_play_custom(const uint8_t *start, const uint8_t *end, va_resample_param_t *playback_param);
int tone_play(tone_type_t type);
/** This API will enable a tone which would be played if dialog is in progress and timer/alarm goes off.
 * Enabling this tone would increase size of binary by around 768 KB */
void tone_enable_larger_tones();
