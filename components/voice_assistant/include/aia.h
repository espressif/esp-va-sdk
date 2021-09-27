// Copyright 2018 Espressif Systems (Shanghai) PTE LTD
// All rights reserved.

#ifndef _AIA_H_
#define _AIA_H_

#include "voice_assistant.h"
#include "auth_delegate.h"
#include <mqtt_client.h>
#include <alexa_smart_home.h>

/* Once assigned by the application, these should not be freed as long as the device is working. */
typedef struct {
    /* Certificate of aws root CA */
    char *aws_root_ca_pem_cert;
    /* Device certificate */
    char *certificate_pem_crt_cert;
    /* Private Key */
    char *private_pem_crt_cert;
    /* AWS Account ID */
    char *aws_account_id;
    /* AWS End Point */
    char *aws_endpoint;
    /* Client id used to communicate with the cloud */
    char *client_id;
} device_config_t;

/** The Alexa Configuration Structure
 */
typedef struct {
    char *device_serial_num;
    char *product_id;
    device_config_t device_config;
} aia_config_t;

/** Initialize Alexa
 *
 * This call must be made after the Wi-Fi connection has been established with the configured Access Point.
 *
 * \param[in] cfg           The Alexa Configuration
 *
 * \return
 *    - 0 on Success
 *    - an error code otherwise
 */
int aia_init(aia_config_t *cfg);

int aia_early_init();

/** Initialise Alexa Bluetooth
 *
 * This enables Alexa's Bluetooth A2DP sink functionality.
 */
int alexa_bluetooth_init();

/** Enable Larger tones.
 *
 * This API will enable a tone which would be played if dialog is in progress and timer/alarm goes off.
 * Enabling this tone would increase size of binary by around 356 KB.
 */
void alexa_tone_enable_larger_tones();
void alexa_auth_delegate_signin(auth_delegate_config_t *cfg);
void alexa_auth_delegate_signout();
void alexa_signin_handler(const char *client_id, const char *refresh_token);
void alexa_signout_handler();

/* These APIs can be called to use the existing AIS mqtt client. */
/* This api givies the status of the aws_iot connection. */
bool alexa_mqtt_is_connected();

/**
 * @brief   Get pointer to Alexa config
 */
aia_config_t *aia_get_cfg();

#endif /*_AIA_H_ */
