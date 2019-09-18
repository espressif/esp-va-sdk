#pragma once

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

//Initializae va_dsp
void va_dsp_init(void);
//Call this api to start streaming audio data from microphones
int va_dsp_tap_to_talk_start();
//API to reset dsp
void va_dsp_reset();
//Call this api to mute(1)/unmute(0) Microphones
void va_dsp_mic_mute(bool mute);
