// Copyright 2018 Espressif Systems (Shanghai) PTE LTD
// All rights reserved.

#pragma once

/* Check Wi-Fi Reset Bit
 *
 * This API checks the wifi_reset bit in NVS.
 * If it is set, then it sets the provisioning status as false, so that the device goes into provisioning mode. It also starts a timer for `WIFI_RESET_TIMER_TIMEOUT` seconds to change the Wi-Fi credentials. If the timer expires before the new Wi-Fi credentials are applied, then the device reboots and tries to connect with the previous credentials itself.
 */
int app_wifi_check_wifi_reset();

/* Initialise Wi-Fi Reset
 *
 * This API registers the callback with `va_button` for wifi_reset.
 */
int app_wifi_init_wifi_reset();

/* Initialise Wi-Fi
 *
 * This API initialises the tcpip_adapter and esp_wifi.
 * This API must be called before calling any other Wi-Fi related APIs.
 */
void app_wifi_init();

/* Start Wi-Fi
 *
 * This API starts the Wi-Fi in Station mode. It must be called after calling `app_wifi_init`.
 */
void app_wifi_start_station();

/* Wait for Wi-Fi connection.
 *
 * Waits for Wi-Fi connection to be completed. This is a blocking call.
 * If Wi-Fi is started using the `app_wifi_start_station` API, then this is unblocked internally when the device gets IP Address.
 */
void app_wifi_wait_for_connection(uint32_t wait);
