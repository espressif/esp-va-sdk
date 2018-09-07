// Copyright 2018 Espressif Systems (Shanghai) PTE LTD
// All rights reserved.

#ifndef _ALERTS_H_
#define _ALERTS_H_

/** Notify to stop the current Alert
 * This API should be called by the application to indicate that the user has
 * stopped an active alert by pressing the on-device button instead of waking up
 * Alexa and saying stop.
 *
 *
 * This is also useful when the device has lost internet
 * connectivity and an alert goes off. The user then presses the on-device button
 * to stop the alert.
 */
void alerts_stop_currently_active();

#endif /* _ALERTS_H_ */
