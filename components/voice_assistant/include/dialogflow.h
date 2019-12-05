// Copyright 2018 Espressif Systems (Shanghai) PTE LTD
// All rights reserved.

#ifndef _DIALOGFLOW_H_
#define _DIALOGFLOW_H_

#include "voice_assistant.h"
#include "session.pb-c.h"

/** Device specific configuration */
typedef struct {
    /** The registered name of the project */
    char *project_name;
    /** The supported device language (default: "en-US") */
    char *device_language;
} dialogflow_device_config_t;

/** The Dialogflow Configuration Structure */
typedef struct {
    /** Configurations for the device */
    dialogflow_device_config_t device_config;
} dialogflow_config_t;

enum dialogflow_conversation_type {
    /** The interaction should continue and mic should be started again. */
    DIALOGFLOW_CONVERSATION_CONTINUE,
    /** The interaction is complete and mic should not be started again. */
    DIALOGFLOW_CONVERSATION_COMPLETE,
    /** The mic should not be started again, but the interaction is also not complete. The user can press the TTT and continue the conversation. */
    DIALOGFLOW_CONVERSATION_STOP,
    /** Used for internal purposes */
    DIALOGFLOW_CONVERSATION_INVALID,
};

/** Initialize Dialogflow
 *
 * This call must be made after the Wi-Fi connection has been established with the configured Access Point.
 *
 * \param[in] cfg The Dialogflow configuration of type dialogflow_config_t
 *
 * \return
 *    - 0 on Success
 *    - an error code otherwise
 */
int dialogflow_init(dialogflow_config_t *cfg);

/** Audio request transcript
 *
 * This callback is executed each time the text transcript of the input audio
 * is received from the Dialogflow server. Multiple transcripts can be received
 * during the course of a single input audio request. The final one will have the is_final bool set to true
 *
 * \param[in] text The current transcript of audio request as interpreted by the Dialogflow server
 * \param[in] is_final If false, it represents an interim result that may change
 *                     If true, the recognizer will not return any further hypotheses about this piece of audio
 */
void dialogflow_app_query_text_transcript(char *text, bool is_final);

/** Dialogflow response data
 *
 * This callback is executed each time the query result is populated by the Dialogflow server
 * and sent back to the device. It is the responsibility of the application to check if all
 * the required entities of an intent are received and then take the necessary action.
 *
 * \param[in] response The response from Dialogflow server of type Google__Cloud__Dialogflow__V2beta1__DetectIntentResponse
 *
 * \return
 *    - App must return 0 if the interaction should continue and mic should be started again.
 *    - App must return 1 if the interaction is complete and mic should not be started again.
 *    - App can return 2 if the mic should not be started again, but the interaction is also not
 *      complete. The user can press the TTT and continue the conversation.
 */
enum dialogflow_conversation_type dialogflow_app_response_data(Google__Cloud__Dialogflow__V2beta1__StreamingDetectIntentResponse *response);

/** Notify the voice assistant of a text recognize event
 *
 * The application can call this function when it wants to initiate communication with the voice assistant through a text.
 *
 * \param[in] buf       Buffer with the text query.
 * \param[in] buf_size  Length of the text query.
 */
int dialogflow_text_recognizer_recognize(char *buf, int buf_size);

#endif /*_DIALOGFLOW_H_ */
