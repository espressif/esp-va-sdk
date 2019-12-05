// Copyright 2018 Espressif Systems (Shanghai) PTE LTD
// All rights reserved.

#pragma once

#include <va_dsp.h>

/** Stream data to the voice assistant
 *
 * Once an application indicates to the voice assistant of a user initiated
 * communication, this API must be called to stream the audio data
 * captured by the microphone. This data must be 16-byte 16KHz sampled
 * audio data.
 *
 * \param[in] data Pointer to a buffer that holds the audio samples
 * \param[in] len The length of the data in the buffer
 */
int speech_recognizer_record(void *data, int len);

/** Signal end of data to the voice assistant
*
* This API must be called when all the data has been sent to the voice assistant.
*/
int speech_recognizer_record_stop();

/** Notify the voice assistant of a recognize event
 *
 * The application should call this function when the user initiates
 * communication with the voice assistant either through a wakeword or through
 * Tap-to-talk.
 *
 * \param[in] ww_length The length of the wakeword, if this is a wakeword driven interaction
 * \param[in] initiator The type of initiator
 */
int speech_recognizer_recognize(int ww_length, enum initiator);
