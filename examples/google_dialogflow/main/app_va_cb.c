// Copyright 2018 Espressif Systems (Shanghai) PTE LTD
// All rights reserved.

#include "stdio.h"
#include <va_led.h>
#include <voice_assistant_app_cb.h>
#include "va_board.h"
#include <media_hal.h>
#include "esp_log.h"
#include <esp_system.h>
#include <esp_heap_caps.h>
#include <dialogflow.h>

static const char *TAG = "[app_va_cb]";
static int prv_led_state = 1000;

static inline int heap_caps_get_free_size_sram()
{
    return heap_caps_get_free_size(MALLOC_CAP_8BIT) - heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
}

void va_app_dialog_states(va_dialog_states_t va_state)
{
    if(prv_led_state != va_state) {
        va_led_set(va_state);
        ESP_LOGI(TAG, "Dialog state is: %d", va_state);
    }
    prv_led_state = va_state;
}

int va_app_volume_is_set(int vol)
{
    volume_to_set = vol;
    va_led_set(VA_SET_VOLUME);
    va_led_set(VA_SET_VOLUME_DONE);
    return 0;
}

int va_app_mute_is_set(va_mute_state_t va_mute_state)
{
    if (va_mute_state == VA_MUTE_ENABLE) {
        va_led_set(VA_SPEAKER_MUTE_ENABLE);
        va_led_set(VA_SET_VOLUME_DONE);
    } else {
        //do nothing
    }
    ESP_LOGI(TAG, "Mute: %d", va_mute_state);
    return 0;
}

enum dialogflow_conversation_type dialogflow_app_response_data(Google__Cloud__Dialogflow__V2beta1__StreamingDetectIntentResponse *response)
{
    int i;
    bool end_interaction = false;
    /* This will set the default as mic start again if the query is not complete. You can change this to DIALOGFLOW_CONVERSATION_STOP if you don't want to start the mic automatically (eg. in case of only text-queries.). */
    enum dialogflow_conversation_type dialogflow_conversation_type = DIALOGFLOW_CONVERSATION_CONTINUE;
    printf("The query text is: %s\n", response->query_result->query_text);
    printf("The fulfillment text is: %s\n", response->query_result->fulfillment_text);

    Google__Protobuf__Struct *entities = response->query_result->parameters;
    if (!entities) {
        /* No response from server, stop the mic, but dont change the session ID. */
        return DIALOGFLOW_CONVERSATION_STOP;
    }

    /* Check if this is the end of the query. And no more data from the user is required. (i.e. The mic needs to be stopped after this query) */
    if (response->query_result) {
        if (response->query_result->intent) {
            if (response->query_result->intent->end_interaction) {
                end_interaction = true;
            }
        } else {
            /* To end the conversation when there is no response from the cloud. Maybe the cloud did not understand anything. */
            dialogflow_conversation_type = DIALOGFLOW_CONVERSATION_COMPLETE;
        }
        if (entities->n_fields == 0 || response->query_result->all_required_params_present == true || end_interaction) {
            dialogflow_conversation_type = DIALOGFLOW_CONVERSATION_COMPLETE;
        }
    }

    /* Print the status of the parameters. */
    printf("Number of entities: %d\n", entities->n_fields);
    for (i = 0; i < entities->n_fields; i++) {
        switch (entities->fields[i]->value->kind_case) {
            case GOOGLE__PROTOBUF__VALUE__KIND_NUMBER_VALUE:
                printf("\t[%d] %s : %f\n", i, entities->fields[i]->key, entities->fields[i]->value->number_value);
                break;
            case GOOGLE__PROTOBUF__VALUE__KIND_STRING_VALUE:
                printf("\t[%d] %s : %s\n", i, entities->fields[i]->key, entities->fields[i]->value->string_value);
                break;
            case GOOGLE__PROTOBUF__VALUE__KIND_BOOL_VALUE:
                printf("\t[%d] %s : %s\n", i, entities->fields[i]->key, entities->fields[i]->value->bool_value ? "true" : "false");
                break;
            default:
                printf("\tUnknown type %d: Please update this debug function\n", entities->fields[i]->value->kind_case);
                break;
        }
    }

    if (dialogflow_conversation_type == DIALOGFLOW_CONVERSATION_COMPLETE) {
        printf("Dialogflow conversation complete");
    } else {
        printf("Dialogflow conversation continued\n");
    }
    return dialogflow_conversation_type;
}

void dialogflow_app_query_text_transcript(char *text, bool is_final)
{
    printf("%s transcript is: %s\n", (is_final != 0 ? "Final" : "Interim"),text);
}

