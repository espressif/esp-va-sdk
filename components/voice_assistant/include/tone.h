// Copyright 2018 Espressif Systems (Shanghai) PTE LTD
// All rights reserved.

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <voice_assistant_app_cb.h>
#include <media_hal_playback.h>

typedef enum {
    TONE_WAKE,
    TONE_WAKE_TOUCH,
    TONE_ENDPOINT,
    TONE_PRIVACY_ON,
    TONE_PRIVACY_OFF,
    TONE_VOLUME,
    TONE_BT_CONNECT,
    TONE_BT_DISCONNECT,
    TONE_ALARM_SHORT,
    TONE_TIMER_SHORT,
    TONE_REMINDER_SHORT,
    TONE_ERROR,
    TONE_MAX,
} tone_type_t;

/** This API will play custom tones provided by the application (should be the part of firmware itself)
 *
 * Please check "Build System" section of ESP-IDF Programming guide to know how to make changes in Makefile or component.mk to embed binary data in firmware and get start and end pointers of the binary.
 * Supported files types: wav
 *
 * \param[in] start Pointer to start of tone in flash
 * \param[in] end Pointer to end of tone in flash
 * \param[in] audio_info Pointer to playback parameters. Please note this needs to point to valid location while the tone is being played. It's recommeded that it is declared as either global or static variable.
 * \return 0 on success, -1 otherwise
 */
int tone_play_custom(const uint8_t *start, const uint8_t *end, media_hal_audio_info_t *audio_info);

/** This API can be called to change the existing tones of tone_type_t (The tone should be the part of firmware itself)
 *
 * Please check "Build System" section of ESP-IDF Programming guide to know how to make changes in Makefile or component.mk to embed binary data in firmware and get start and end pointers of the binary.
 * This API should be called after calling the respective init for the voice_assistant i.e. <voice_assistant>_init().
 * Supported file types: wav
 *
 * \param[in] start Pointer to start of tone in flash
 * \param[in] end Pointer to end of tone in flash
 * \param[in] audio_info Pointer to playback parameters. Please note this needs to point to valid location while the tone is being played. It's recommeded that it is declared as either global or static variable.
 */
void tone_set_custom(tone_type_t type, const uint8_t *start, const uint8_t *end, media_hal_audio_info_t *audio_info);
int tone_play(tone_type_t type);
esp_err_t tone_set_sor_state(bool enabled);
esp_err_t tone_set_eor_state(bool enabled);
bool tone_get_sor_state();
bool tone_get_eor_state();

