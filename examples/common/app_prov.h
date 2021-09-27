// Copyright 2018 Espressif Systems (Shanghai) PTE LTD
// All rights reserved.

#pragma once

#include <stdbool.h>

/* Get Provisioning Status
 *
 * This just returns the provisioning status of the device, i.e. if it is provisioned already or not. This status is set on calling the `app_prov_init` API and it can be over-written with the `app_prov_set_provisioning_status` API. 
 */
bool app_prov_get_provisioning_status();

/* Set Provisioning Status
 *
 * This API can be used to over-write the status which is set after calling the `app_prov_init` API.
 */
void app_prov_set_provisioning_status(bool provisioning_status);

/* Wait for provisioning
 *
 * Waits for provisioning to be completed. This is a blocking call.
 * It can be unblocked by calling the `app_prov_stop_provisioning` API.
 * If provisioning has been started, then this will be unblocked internally once provisioning is complete.
 */
void app_prov_wait_for_provisioning();

/* Finish Provisioning
 *
 * Deinitialises the wifi_provisioning manager.
 * Also unblocks the `app_prov_wait_for_provisioning` API.
 */
void app_prov_stop_provisioning();

/* Start Provisioning
 *
 * The application can check the status of provisioning through `app_prov_get_provisioning_status` and then call this API accordingly.
 *
 * \param[in] service_name The name with which the provisioning is started.
 * \param[in] data The configurations which are necessary for the voice_assistant to be initialised. This is passed to the SDK.
 */
void app_prov_start_provisionig(const char *service_name, void *data);

/* Provisioning Initialise
 *
 * Initialises the wifi_provisioning manager from IDF.
 * This also sets the provisioning status of the device, i.e. if it is provisioned already or not. This status can be checked by calling the `app_prov_get_provisioning_status` API.
 */
void app_prov_init();
