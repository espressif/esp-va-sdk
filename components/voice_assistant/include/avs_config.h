// Copyright 2018 Espressif Systems (Shanghai) PTE LTD
// All rights reserved.

#ifndef _AVS_CONFIG_H_
#define _AVS_CONFIG_H_

/** Alexa Provisioning Init
 *
 * This API must be called in the application after wifi_init() but before wifi_mgr_prov_init().
 * This internally adds the endpoint for Alexa for provisioning.
 *
 * @param[in] user_data pointer to the voice assistant configuration.
 */
esp_err_t alexa_provisioning_init(void *user_data);

/* For internal use */
int avs_config_data_handler(uint32_t session_id, const uint8_t *inbuf, ssize_t inlen, uint8_t **outbuf, ssize_t *outlen, void *priv_data);

#endif /* _AVS_CONFIG_H_ */
