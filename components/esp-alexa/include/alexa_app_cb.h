// Copyright 2018 Espressif Systems (Shanghai) PTE LTD
// All rights reserved.

#ifndef _ALEXA_APP_CB_H_
#define _ALEXA_APP_CB_H_

#include <unistd.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Alexa Dialog States
 */
typedef enum {
    /** Alexa is Thinking */
    ALEXA_THINKING = 1 << 0,
    /** Alexa is Speaking */
    ALEXA_SPEAKING = 1 << 1,
    /** Alexa is Listening */
    ALEXA_LISTENING = 1 << 2,
    /** Alexa is Idle */
    ALEXA_IDLE = 1 << 3,
    /** End of current states markers */
    ALEXA_END_STATES = 1 << 4,
    /** Alexa diaglog state Max*/
    ALEXA_DIALOG_STATES_MAX,
} alexa_dialog_states_t;

/** Alexa Mute States
 */
typedef enum {
    /** Mute is disabled */
    ALEXA_MUTE_DISABLE = (ALEXA_DIALOG_STATES_MAX + 1),
    /** Mute is enabled */
    ALEXA_MUTE_ENABLE,
} alexa_mute_state_t;



/** Speaker Alexa Mute States
 */
typedef enum {
    /** Speaker Mute is disabled */
    ALEXA_SPEAKER_MUTE_ENABLE = (ALEXA_MUTE_ENABLE + 1),
    /** Speaker Mute is enabled */
    ALEXA_SPEAKER_MUTE_DISABLE,
} alexa_speaker_mute_state_t;


/** Set volume
 */
typedef enum {
    /** Set volume  */
    ALEXA_SET_VOLUME = (ALEXA_SPEAKER_MUTE_DISABLE + 1),
} alexa_set_volume_t;

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
} alexa_alert_types_t;

/** Alexa Alert States
*/
typedef enum {
    /** Corresponding alert is active */
    ALEXA_ALERT_ENABLE,
    /** Corresponding alert is over */
    ALEXA_ALERT_DISABLE,
} alexa_alert_state_t;

/*
 * Alexa resample i2s parameters
 */
typedef struct {
    int alexa_resample_freq;
    int alexa_resample_ch;
} alexa_resample_param_t;

/** Dialog State callback
 *
 * This callback is executed when the dialog state changes. Please
 * refer to \ref alexa_dialog_state_t for the various dialog
 * states. The application is expected to show any LED indicators as
 * desired for this Alexa state.
 *
 * \note The callback may be called from different thread contexts
 * from with the Alexa submodule. It is advised that this callback
 * doesn't take much stack or time during execution.
 *
 * \param[in] alexa_states The current dialog state
 */
void alexa_app_dialog_states(alexa_dialog_states_t alexa_states);

/** Stop Speech callback
 *
 * This callback is executed when Alexa wishes to stop the microphone
 * capture. The application is expected to stop the microphone and
 * stop streaming microphone data to AVS.
 */
int alexa_app_speech_stop();

/** Start Speech callback
 *
 * This callback is executed when Alexa wishes to start the microphone
 * capture. The application is expected to start the microphone and
 * start streaming micro-phone data to AVS. This call is typically
 * called in response to an ExpectSpeech directive, where Alexa queries
 * the user for additional information.
 */
int alexa_app_speech_start();

/** Set Volume callback
 *
 * This callback is executed when Alexa wishes to change the output
 * volume of the speaker.
 */
int alexa_app_set_volume(int vol);

/** Set Mute callback
 *
 * This callback is executed when Alexa wishes to toggle the mute
 * state of the speaker.
 */
int alexa_app_set_mute(alexa_mute_state_t alexa_mute_state);

/** Set Alert callback
*
* This callback is executed when Alexa wishes to notify about an
* alert or a notification that has started or if it has ended.
*/
int alexa_app_raise_alert(alexa_alert_types_t alexa_alert_type, alexa_alert_state_t alexa_alert_state);

/*
 *This is a callback function to resample the playback signal which is coming from sys_playback
 *The resampled signal is then passed to the codec/dsp/amplifier
 */
int alexa_app_playback_data(alexa_resample_param_t *alexa_resample_param, void *buf, ssize_t len);


#ifdef __cplusplus
}
#endif

#endif /* _ALEXA_APP_CB_H_ */
