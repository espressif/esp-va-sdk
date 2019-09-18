// Copyright 2018 Espressif Systems (Shanghai) PTE LTD
// All rights reserved.

#ifndef _VOICE_ASSISTANT_APP_CB_H_
#define _VOICE_ASSISTANT_APP_CB_H_

#include <unistd.h>

/** Voice Assistant Dialog States
 */
typedef enum {
    /** Voice Assistant is Thinking */
    VA_THINKING = 1 << 0,
    /** Voice Assistant is Speaking */
    VA_SPEAKING = 1 << 1,
    /** Voice Assistant is Listening */
    VA_LISTENING = 1 << 2,
    /** Voice Assistant is Idle */
    VA_IDLE = 1 << 3,
    /** End of current states markers */
    VA_END_STATES = 1 << 4,
    /** Voice Assistant diaglog state Max*/
    VA_DIALOG_STATES_MAX,
} va_dialog_states_t;

/** Voice Assistant Mute States
 */
typedef enum {
    /** Mute is disabled */
    VA_MUTE_DISABLE = (VA_DIALOG_STATES_MAX + 1),
    /** Mute is enabled */
    VA_MUTE_ENABLE,
} va_mute_state_t;

/** Speaker Voice Assistant Mute States
 */
typedef enum {
    /** Speaker Mute is disabled */
    VA_SPEAKER_MUTE_ENABLE = (VA_MUTE_ENABLE + 1),
    /** Speaker Mute is enabled */
    VA_SPEAKER_MUTE_DISABLE,
} va_speaker_mute_state_t;

/** Set volume
 */
typedef enum {
    /** Set volume  */
    VA_SET_VOLUME = (VA_SPEAKER_MUTE_DISABLE + 1),
    VA_SET_VOLUME_DONE,
} va_set_volume_t;

/** Alexa Alert Types
*/
typedef enum {
    /** Alarm alert */
    ALEXA_ALERT_ALARM = 0,
    /** Reminder alert */
    ALEXA_ALERT_REMINDER,
    /** Timer alert */
    ALEXA_ALERT_TIMER,
    /** Notification alert */
    ALEXA_ALERT_NOTIFICATION,
    /** Short alert */
    ALEXA_ALERT_SHORT,
} alexa_alert_types_t;

/** Alexa Alert States
*/
typedef enum {
    /** Corresponding alert is active */
    ALEXA_ALERT_ENABLE,
    /** Corresponding alert is over */
    ALEXA_ALERT_DISABLE,
} alexa_alert_state_t;

/** Dialog State callback
 *
 * This callback is executed when the dialog state changes. Please
 * refer to \ref va_dialog_state_t for the various dialog
 * states. The application is expected to show any LED indicators as
 * desired for this Voice Assistant state.
 *
 * \note The callback may be called from different thread contexts
 * from with the Voice Assistant submodule. It is advised that this callback
 * doesn't take much stack or time during execution.
 *
 * \param[in] va_states The current dialog state
 */
void va_app_dialog_states(va_dialog_states_t va_states);

/** Stop Speech callback
 *
 * This callback is executed when the voice assistant wishes to stop the microphone
 * capture. The application is expected to stop the microphone and
 * stop streaming microphone data to AVS.
 */
int va_app_speech_stop();

/** Start Speech callback
 *
 * This callback is executed when the voice assistant wishes to start the microphone
 * capture. The application is expected to start the microphone and
 * start streaming micro-phone data to AVS. This call is typically
 * called in response to an ExpectSpeech directive, where the voice assistant queries
 * the user for additional information.
 */
int va_app_speech_start();

/** Notify playback to DSP
 *
 * This should be called when voice assistant is about to write some data to playback
 * interface
 */
int va_app_playback_starting();

/** Set Volume callback
 *
 * This callback is executed when the voice assistant has changed the output
 * volume of the speaker.
 */
int va_app_volume_is_set(int vol);

/** Set Mute callback
 *
 * This callback is executed when the voice assistant wishes to toggle the mute
 * state of the speaker.
 */
int va_app_mute_is_set(va_mute_state_t va_mute_state);

/** Set Alert callback
*
* This callback is executed when Alexa wishes to notify about an
* alert or a notification that has started or if it has ended.
*/
int alexa_app_raise_alert(alexa_alert_types_t alexa_alert_type, alexa_alert_state_t alexa_alert_state);

#endif /* _VOICE_ASSISTANT_APP_CB_H_ */
