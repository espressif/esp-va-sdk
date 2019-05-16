// Copyright 2018 Espressif Systems (Shanghai) PTE LTD
// All rights reserved.

#ifndef _SPEAKER_H_
#define _SPEAKER_H_

/** Notify Speaker Volume Change
 *
 * This API should be called by the application to notify voice assistant that a
 * user has changed the speaker volume using on-device buttons.
 *
 * Typically, a user presses a button multiple times to change the
 * volumes. In order to avoid sending multiple requests to the voice assistant, the
 * application must take care that the multiple button presses are
 * absorbed by the application and a call with the final volume
 * setting be made.
 *
 * \param[in] vol Volume represented in percentage. Allowed range is 0-100.
 *
 */
int speaker_notify_vol_changed(int vol);

/** Change the speaker volume
 *
 * This API should be called by the application to change the volume of the speaker.
 * The volume is changed instantly. Also a callback event for volume changed is sent to
 * the voice assistant.
 *
 * \param[in] vol Volume represented in percentage. Allowed range is 0-100.
 *
 */
int speaker_set_vol(int vol);

/** Get the speaker volume
 *
 * This API returns the currrent speaker volume. The value is in
 * percentage and its range is 0-100.
 *
 */
int speaker_get_vol();

/** Change the speaker mute state
 *
 * This API should be called by the application to change the mute
 * state of the speaker. The mute is enabled/disabled instantly. Also
 * a callback event for mute changed is sent to the voice assistant.
 *
 * \param[in] vol Volume represented in percentage. Allowed range is 0-100.
 *
 */
int speaker_set_mute(bool mute);

/** Get the mute state
 *
 * This API returns the currrent mute state. The value is true is mute is enabled and
 * false if mute is disabled.
 *
 */
bool speaker_get_mute();

#endif /* _SPEAKER_H_ */
