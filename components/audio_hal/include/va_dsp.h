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

void va_dsp_init(void);
int va_dsp_tap_to_talk_start();
void va_dsp_reset();
void va_dsp_mic_mute(bool mute);
