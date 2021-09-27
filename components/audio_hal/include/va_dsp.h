/*
 * ESPRESSIF MIT License
 *
 * Copyright (c) 2019 <ESPRESSIF SYSTEMS (SHANGHAI) CO., LTD>
 *
 * Permission is hereby granted for use on all ESPRESSIF SYSTEMS products, in which case,
 * it is free of charge, to any person obtaining a copy of this software and associated
 * documentation files (the "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the Software is furnished
 * to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#pragma once

#include <stdbool.h>

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
 * @brief       Mic mute notify callback
 *
 * @param[in]   mute    Mic mute status
 */
typedef void (*va_dsp_notify_mute_cb_t)(bool mute);

/**
 * @brief       Initialize dsp.
 *
 * @note        One also need to provide callbacks for dsp recognize
 *              and data_record as parameters to this function.
 */
void va_dsp_init(va_dsp_recognize_cb_t va_dsp_recognize_cb, va_dsp_record_cb_t va_dsp_record_cb, va_dsp_notify_mute_cb_t va_dsp_mute_notify_cb);

/**
 * @brief   Notify playback stop event to DSP
 *
 * @note    This should be called when voice assistant has done writing to
 *          playback interface. va_dsp can take appropriate actions in this callback
 *
 */
int va_dsp_playback_stopped();

/**
 * @brief   Notify playback to DSP
 *
 * @note    This should be called when voice assistant is about to write some data to
 *          playback interface. va_dsp can take appropriate actions in this callback
 */
int va_dsp_playback_starting();

/**
 * @brief   Notify ongoing playback to DSP
 *
 * @note    This should be called when voice assistant is writing data to
 *          playback interface. va_dsp can take appropriate actions in this callback
 */
int va_dsp_playback_ongoing();

//Call this api to start streaming audio data from microphones
int va_dsp_tap_to_talk_start();

//API to reset dsp
void va_dsp_reset();

//Call this api to mute(1)/unmute(0) Microphones
void va_dsp_mic_mute(bool mute);
