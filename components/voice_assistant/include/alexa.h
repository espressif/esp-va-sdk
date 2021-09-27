// Copyright 2018 Espressif Systems (Shanghai) PTE LTD
// All rights reserved.

#ifndef _ALEXA_H_
#define _ALEXA_H_

#include <esp_err.h>
#include "voice_assistant.h"
#include "auth_delegate.h"
#include <alexa_smart_home.h>

/** Enable Larger tones.
 *
 * This API will enable a tone which would be played if dialog is in progress and timer/alarm goes off.
 * Enabling this tone would increase size of binary by around 356 KB.
 */
void alexa_tone_enable_larger_tones();

/** Change Alexa language.
 *
 * This API can be used to change the primary locale i.e language and/or accent for Alexa. Supported locales can be found here: https://developer.amazon.com/en-US/docs/alexa/alexa-voice-service/system.html#locales. Default is "en-IN"
 * API should be called after alexa_init().
 *
 * @param locale code string of primary locale. Valid locale IDs are ones listed on the link above.
 * @return 0 on success
 *         -1 on failure
 */
int alexa_change_locale(const char *locale);

/** Set secondary language
 *
 * This API can be used to set second language of Alexa which complements the primary language. Supported locale combinations can be found here: https://developer.amazon.com/en-US/docs/alexa/alexa-voice-service/system.html#localecombinations. No secondary language is enabled by default.
 * Ideally the function should be called only after setting the primary locale.
 * API should be called after alexa_init().
 *
 * @param locale code string of secondary locale. Valid locale IDs are ones listed on the link above.
 * @return 0 on success
 *         -1 on failure
 */
int alexa_set_secondary_locale(const char *locale);

/** Set locale combination
 *
 * This API can be used to set the primary-secondary locale combination. Supported locale combinations can be found here: https://developer.amazon.com/en-US/docs/alexa/alexa-voice-service/system.html#localecombinations. No combination is set by default.
 * API should be called after alexa_init().
 *
 * @param primary_locale code string of primary locale.
 * @param secondary_locale code string of secondary locale.
 * @return 0 on success
 *         -1 on failure
 */
int alexa_set_locale_comb(const char *primary_locale, const char *secondary_locale);

/** Application callback for a successful Alexa sign in */
typedef void (*alexa_app_signin_handler_t)(void *data);

/** Application callback for a successful Alexa sign out */
typedef void (*alexa_app_signout_handler_t)(void *data);

/** Initialize authentication module for Alexa
 *
 * @param: signin_handler Callback called upon successful sign in
 * @param: signout_handler Callback called upong successful sign out
 */
void alexa_auth_delegate_init(alexa_app_signin_handler_t signin_handler, alexa_app_signout_handler_t signout_handler);
void alexa_auth_delegate_signin(auth_delegate_config_t *cfg);
void alexa_auth_delegate_signout();

/**
 * Manufacturer specific info about the device.
 * */
typedef struct {
    /** Name of the manufacturer. Default is "Espressif" */
    char *manufacturer_name;
    /** One-liner device description */
    char *device_description;
    /** Friendly name of the device. This is to identify the device on the app. */
    /* XXX: App modifiable? */
    char *friendly_name;
    /** Device's category from display_category_t. Default is OTHER */
    smart_home_device_type_t device_category;
    /** Model name. Default is "ESP32" */
    char *model_name;
    /** Unique ID of the device. Default is device's MAC address */
    char *custom_unique_id;
} alexa_manufacturer_device_info_t;
/**
 * The Alexa Configuration Structure
 */
typedef struct {
    char *device_serial_num;
    char *product_id;
    alexa_manufacturer_device_info_t device_info;
} alexa_config_t;

/**
 * @brief Check and init the device in BT standalone mode.
 *
 * The device will become BT-SINK speaker only. All Alexa functionality is disabled in this case.
 *
 * \return
 *      - ESP_OK on success
 *      - ESP_FAIL if device is not set to be run in Alexa+BT mode.
 */
esp_err_t alexa_bt_only_mode_init();

/**
 * @brief Restart the device in BT-only mode.
 *
 * The device will become BT-SINK speaker only. All Alexa functionality is disabled in this case.
 *
 * \return
 *      - ESP_OK on success
 *      - ESP_FAIL if device is already in BT-only mode.
 *
 * @note the api has no effect if a2dp_sink is not enabled using `alexa_bt_a2dp_sink_init`.
 */
int alexa_enable_bt_only_mode();

/**
 * @brief Restart the device in Alexa+BT mode.
 *
 * The device will have all the functionality of Alexa+BT.
 *
 * \return
 *      - ESP_OK on success
 *      - ESP_FAIL if the device is already in Alexa+BT mode
 *
 * @note the api has no effect if a2dp_sink is not enabled using `alexa_bt_a2dp_sink_init`.
 */
int alexa_disable_bt_only_mode();

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
int alexa_init(alexa_config_t *alexa_cfg);

int alexa_early_init();

/**
 * Enable BT A2DP sink and source
 *
 * Function enables BT A2DP sink and source functionality and registers it with Alexa. Function should be called before
 * the call to alexa_init()
 *
 * @note this function just sets a2dp_sink to be inited. Actual initialization is done by `alexa_init()`
 */
void alexa_bt_init();

/**
 * Initialize Alexa configuration with default values.
 *
 * Should be called after tcpip_adapter_init()
 */
void alexa_init_config(alexa_config_t *alexa_cfg);
#endif /*_ALEXA_H_ */
