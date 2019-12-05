#pragma once

/** Initiator Type
 */
enum initiator {
    /** Communication was initiated using a Wake-Word */
    WAKEWORD,
    /** Communication was initiated using a Tap-to-Talk event */
    TAP,
    /** This is not yet supported */
    HOLD_AND_TALK,
    /** Communication was initatied by voice assistant due to ExpectSpeech. The application will never send this value */
    EXPECT_SPEECH,
    /** Communication was initiated with text input. Used in Google Dialogflow */
    TEXT,
};

enum va_dsp_events {
    IDLE,
    WW,
    TAP_TO_TALK,
    GET_AUDIO,
    START_MIC,
    STOP_MIC,
    MUTE,
    UNMUTE,
    POWER_SAVE,
};

struct dsp_event_data {
    enum va_dsp_events event;
};

/**
 * @brief       DSP record callback
 *
 * @param[in]   data    Pointer to the buffer holdind recorded data
 * @param[in]   len     Length of the data
 */
typedef int (*va_dsp_record_cb_t) (void *data, int len);

/**
 * @brief       Wakeup recognize callback
 *
 * @param[in]   ww_length     Length of data with wakeword
 * @param[in]   init_type     Type of wakeup given by `initiator` enum
 */
typedef int (*va_dsp_recognize_cb_t) (int ww_length, enum initiator init_type);

/**
 * @brief       Initialize dsp.
 *
 * Note: One also need to provide callbacks for dsp recognize and data_record as parameters to this function.
 */
void va_dsp_init(va_dsp_recognize_cb_t va_dsp_recognize_cb, va_dsp_record_cb_t va_dsp_record_cb);

//Call this api to start streaming audio data from microphones
int va_dsp_tap_to_talk_start();
//API to reset dsp
void va_dsp_reset();
//Call this api to mute(1)/unmute(0) Microphones
void va_dsp_mic_mute(bool mute);
