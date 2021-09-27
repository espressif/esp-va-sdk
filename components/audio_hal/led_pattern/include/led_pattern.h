// Copyright 2018 Espressif Systems (Shanghai) PTE LTD
// All rights reserved.

#ifndef _LED_PATTERN_H_
#define _LED_PATTERN_H_

#include <stdint.h>
#include <stdbool.h>
#include <esp_err.h>

/** Maximum number of LEDs supported */
#define MAX_LEDS 12

/** One complete LED state comprising of the RGB values for all the LEDs. */
typedef struct {
    /** Time for which the LEDs will stay in this state. (in ms) */
    int led_state_delay;
    /** RGB values for all the LEDs */
    uint32_t led_state_val[MAX_LEDS];
} led_pattern_state_t;

/** One complete LED pattern */
typedef struct {
    /** Number of LED states in this pattern */
    uint8_t led_states_count;
    /** Array of all the LED states */
    led_pattern_state_t *led_states;
} led_pattern_config_t;

/** List of patterns
 *
 * Patterns for which '(looping)' is mentioned are looped until the state changes. This is according to the voice assistant's implementation.
 */
typedef enum {
    /** Setup mode */
    LED_PATTERN_SETUP,
    /** First bootup pattern */
    LED_PATTERN_BOOTUP_1,
    /** Second bootup pattern (looping) */
    LED_PATTERN_BOOTUP_2,
    /** OTA update on-going */
    LED_PATTERN_OTA,
    /** Error */
    LED_PATTERN_ERROR,
    /** Idle (LEDs off). This is a single LED state in which all the LEDs are off. */
    LED_PATTERN_OFF,

    /** Listening state enter */
    LED_PATTERN_LISTENING_ENTER,
    /** Listening state active (looping). This is generally a single LED state which is generally the last state of LED_PATTERN_LISTENING_ENTER. */
    LED_PATTERN_LISTENING_ONGOING,
    /** Listening state exit */
    LED_PATTERN_LISTENING_EXIT,
    /** Thinking state (looping) */
    LED_PATTERN_THINKING,
    /** Speaking state (looping) */
    LED_PATTERN_SPEAKING,

    /** Microphone off enter */
    LED_PATTERN_MIC_OFF_ENTER,
    /** Microphone off active (looping). This is generally a single LED state which is generally the last state of LED_PATTERN_MIC_OFF_ENTER. */
    LED_PATTERN_MIC_OFF_ONGOING,
    /** Microphone off exit */
    LED_PATTERN_MIC_OFF_EXIT,
    /** Current speaker volume level. The LED state corresponds to the volume level. This generally has 20 LED states corresponding to the 20 volume levels. */
    LED_PATTERN_SPEAKER_VOL,
    /** Speaker muted or set to volume 0 */
    LED_PATTERN_SPEAKER_MUTE,

    /** Alert (looping) */
    LED_PATTERN_ALERT,
    /** Short alert */
    LED_PATTERN_ALERT_SHORT,
    /** New notification */
    LED_PATTERN_NOTIFICATION_NEW,
    /** Notification present (looping) */
    LED_PATTERN_NOTIFICATION_ONGOING,
    /** Do not disturb */
    LED_PATTERN_DO_NOT_DISTURB,
    /** Bluetooth connected */
    LED_PATTERN_BT_CONNECT,
    /** Bluetooth disconnected */
    LED_PATTERN_BT_DISCONNECT,

    /** Maximum LED patterns */
    LED_PATTERN_MAX,
} led_pattern_t;

/** Get LED pattern config
 *
 * This API populates the LED pattern config.
 *
 * @param[out] led_pattern_config An array of all the LED patterns.
 *
 * @return ESP_OK on success.
 * @return error in case of failure.
 */
esp_err_t led_pattern_get_config(led_pattern_config_t **led_pattern_config);

/** Initialization check
 *
 * This API checks if LED pattern has been initialized.
 *
 * @return true if initialization is done.
 * @return false if initialization is not done.
 */
bool led_pattern_is_init_done();

/** Initialise LED pattern
 *
 * The LED pattern which is initialised depends on the LED_PATTERN_PATH set.
 */
void led_pattern_init();

#endif /* _LED_PATTERN_H_ */
