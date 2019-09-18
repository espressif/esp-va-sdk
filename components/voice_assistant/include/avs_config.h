// Copyright 2018 Espressif Systems (Shanghai) PTE LTD
// All rights reserved.

#ifndef _AVS_CONFIG_H_
#define _AVS_CONFIG_H_

#include <conn_mgr_prov.h>

/** Callback event for provisioning AVS configuration
 *
 * This api can be set as the event_cb in conn_mgr_prov_t while starting provisioning if the application
 * wants to set the alexa_config_t via the companion app. The cb_user_data in conn_mgr_prov_t must also be 
 * set to an allocated alexa_config_t struct pointer.
 *
 */
int alexa_conn_mgr_prov_cb(void *user_data, conn_mgr_cb_event_t event);
int avs_config_data_handler(uint32_t session_id, const uint8_t *inbuf, ssize_t inlen, uint8_t **outbuf, ssize_t *outlen, void *priv_data);

#endif /* _AVS_CONFIG_H_ */
