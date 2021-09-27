// Copyright 2018 Espressif Systems (Shanghai) PTE LTD
// All rights reserved.

#include <esp_err.h>
#include <esp_log.h>
#include <led_pattern.h>

const led_pattern_state_t led_hollow_listening_enter[] = {
	{50, {0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000}},
};

const led_pattern_state_t led_hollow_listening_ongoing[] = {
    {50, {0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000}},
};

const led_pattern_state_t led_hollow_speaking[] = {
	{50, {0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000}},
};

const led_pattern_state_t led_hollow_mic_off_exit[] = {
	{50, {0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000}},
};

const led_pattern_state_t led_hollow_mic_off_ongoing[] = {
	{50, {0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000}},
};

const led_pattern_state_t led_hollow_mic_off_enter[] = {
	{50, {0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000}},
};

const led_pattern_state_t led_hollow_listening_exit[] = {
	{50, {0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000}},
};

const led_pattern_state_t led_hollow_error[] = {
	{50, {0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000}},
};

const led_pattern_state_t led_hollow_bt_connect[] = {
	{50, {0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000}},
};

const led_pattern_state_t led_hollow_bt_disconnect[] = {
	{50, {0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000}},
};

const led_pattern_state_t led_hollow_notification_ongoing[] = {
	{50, {0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000}},
};

const led_pattern_state_t led_hollow_notification_new[] = {
	{50, {0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000}},
};

const led_pattern_state_t led_hollow_thinking[] = {
	{50, {0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000F}},
};

const led_pattern_state_t led_hollow_alrt_short[] = {
	{50, {0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000}},
};

const led_pattern_state_t led_hollow_alrt[] = {
	{50, {0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000}},
};

const led_pattern_state_t led_hollow_bootup_1[] = {
    {50, {0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000}},
};

const led_pattern_state_t led_hollow_bootup_2[] = {
    {50, {0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000}},
};

const led_pattern_state_t led_hollow_speaker_mute[] = {
    {50, {0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000}},
};

const led_pattern_state_t led_hollow_speaker_vol[] = {
    {50, {0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000}},
    {50, {0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000}},
    {50, {0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000}},
    {50, {0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000}},
    {50, {0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000}},
    {50, {0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000}},
    {50, {0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000}},
    {50, {0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000}},
    {50, {0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000}},
    {50, {0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000}},
    {50, {0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000}},
    {50, {0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000}},
    {50, {0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000}},
    {50, {0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000}},
    {50, {0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000}},
    {50, {0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000}},
    {50, {0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000}},
    {50, {0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000}},
    {50, {0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000}},
    {50, {0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000}},
};

const led_pattern_state_t led_hollow_off[] = {
    {50, {0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000}},
};

const led_pattern_state_t led_hollow_setup_mode[] = {
    {50, {0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000}},
};

const led_pattern_state_t led_hollow_ota[] = {
    {50, {0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000}},
};

const led_pattern_state_t led_hollow_do_not_disturb[] = {
	{50, {0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000,0x000000}},
};

static const char *TAG = "led_pattern";

static bool is_init_done = false;
led_pattern_config_t led_hollow_conf[LED_PATTERN_MAX];

esp_err_t led_pattern_get_config(led_pattern_config_t **led_pattern_config)
{
    if (is_init_done == false) {
        ESP_LOGE(TAG, "LED pattern not initialized");
        return ESP_FAIL;
    }
    *led_pattern_config = led_hollow_conf;
    return ESP_OK;
}

bool led_pattern_is_init_done()
{
    return is_init_done;
}

void led_pattern_init()
{
    led_hollow_conf[LED_PATTERN_BOOTUP_1].led_states_count = sizeof(led_hollow_bootup_1) / sizeof(led_hollow_bootup_1[0]);
    led_hollow_conf[LED_PATTERN_BOOTUP_1].led_states = (led_pattern_state_t *)led_hollow_bootup_1;

    led_hollow_conf[LED_PATTERN_BOOTUP_2].led_states_count = sizeof(led_hollow_bootup_2) / sizeof(led_hollow_bootup_2[0]);
    led_hollow_conf[LED_PATTERN_BOOTUP_2].led_states = (led_pattern_state_t *)led_hollow_bootup_2;

    led_hollow_conf[LED_PATTERN_LISTENING_ENTER].led_states_count = sizeof(led_hollow_listening_enter) / sizeof(led_hollow_listening_enter[0]);
    led_hollow_conf[LED_PATTERN_LISTENING_ENTER].led_states = (led_pattern_state_t *)led_hollow_listening_enter;

    led_hollow_conf[LED_PATTERN_LISTENING_ONGOING].led_states_count = sizeof(led_hollow_listening_ongoing) / sizeof(led_hollow_listening_ongoing[0]);
    led_hollow_conf[LED_PATTERN_LISTENING_ONGOING].led_states = (led_pattern_state_t *)led_hollow_listening_ongoing;

    led_hollow_conf[LED_PATTERN_LISTENING_EXIT].led_states_count = sizeof(led_hollow_listening_exit) / sizeof(led_hollow_listening_exit[0]);
    led_hollow_conf[LED_PATTERN_LISTENING_EXIT].led_states = (led_pattern_state_t *)led_hollow_listening_exit;

    led_hollow_conf[LED_PATTERN_SPEAKER_VOL].led_states_count = sizeof(led_hollow_speaker_vol) / sizeof(led_hollow_speaker_vol[0]);
    led_hollow_conf[LED_PATTERN_SPEAKER_VOL].led_states = (led_pattern_state_t *)led_hollow_speaker_vol;

    led_hollow_conf[LED_PATTERN_SPEAKER_MUTE].led_states_count = sizeof(led_hollow_speaker_mute) / sizeof(led_hollow_speaker_mute[0]);
    led_hollow_conf[LED_PATTERN_SPEAKER_MUTE].led_states = (led_pattern_state_t *)led_hollow_speaker_mute;

    led_hollow_conf[LED_PATTERN_SPEAKING].led_states_count = sizeof(led_hollow_speaking) / sizeof(led_hollow_speaking[0]);
    led_hollow_conf[LED_PATTERN_SPEAKING].led_states = (led_pattern_state_t *)led_hollow_speaking;

    led_hollow_conf[LED_PATTERN_MIC_OFF_EXIT].led_states_count = sizeof(led_hollow_mic_off_exit) / sizeof(led_hollow_mic_off_exit[0]);
    led_hollow_conf[LED_PATTERN_MIC_OFF_EXIT].led_states = (led_pattern_state_t *)led_hollow_mic_off_exit;

    led_hollow_conf[LED_PATTERN_MIC_OFF_ONGOING].led_states_count = sizeof(led_hollow_mic_off_ongoing) / sizeof(led_hollow_mic_off_ongoing[0]);
    led_hollow_conf[LED_PATTERN_MIC_OFF_ONGOING].led_states = (led_pattern_state_t *)led_hollow_mic_off_ongoing;

    led_hollow_conf[LED_PATTERN_MIC_OFF_ENTER].led_states_count = sizeof(led_hollow_mic_off_enter) / sizeof(led_hollow_mic_off_enter[0]);
    led_hollow_conf[LED_PATTERN_MIC_OFF_ENTER].led_states = (led_pattern_state_t *)led_hollow_mic_off_enter;

    led_hollow_conf[LED_PATTERN_ERROR].led_states_count = sizeof(led_hollow_error) / sizeof(led_hollow_error[0]);
    led_hollow_conf[LED_PATTERN_ERROR].led_states = (led_pattern_state_t *)led_hollow_error;

    led_hollow_conf[LED_PATTERN_BT_CONNECT].led_states_count = sizeof(led_hollow_bt_connect) / sizeof(led_hollow_bt_connect[0]);
    led_hollow_conf[LED_PATTERN_BT_CONNECT].led_states = (led_pattern_state_t *)led_hollow_bt_connect;

    led_hollow_conf[LED_PATTERN_BT_DISCONNECT].led_states_count = sizeof(led_hollow_bt_disconnect) / sizeof(led_hollow_bt_disconnect[0]);
    led_hollow_conf[LED_PATTERN_BT_DISCONNECT].led_states = (led_pattern_state_t *)led_hollow_bt_disconnect;

    led_hollow_conf[LED_PATTERN_NOTIFICATION_ONGOING].led_states_count = sizeof(led_hollow_notification_ongoing) / sizeof(led_hollow_notification_ongoing[0]);
    led_hollow_conf[LED_PATTERN_NOTIFICATION_ONGOING].led_states = (led_pattern_state_t *)led_hollow_notification_ongoing;

    led_hollow_conf[LED_PATTERN_NOTIFICATION_NEW].led_states_count = sizeof(led_hollow_notification_new) / sizeof(led_hollow_notification_new[0]);
    led_hollow_conf[LED_PATTERN_NOTIFICATION_NEW].led_states = (led_pattern_state_t *)led_hollow_notification_new;

    led_hollow_conf[LED_PATTERN_THINKING].led_states_count = sizeof(led_hollow_thinking) / sizeof(led_hollow_thinking[0]);
    led_hollow_conf[LED_PATTERN_THINKING].led_states = (led_pattern_state_t *)led_hollow_thinking;

    led_hollow_conf[LED_PATTERN_ALERT_SHORT].led_states_count = sizeof(led_hollow_alrt_short) / sizeof(led_hollow_alrt_short[0]);
    led_hollow_conf[LED_PATTERN_ALERT_SHORT].led_states = (led_pattern_state_t *)led_hollow_alrt_short;

    led_hollow_conf[LED_PATTERN_ALERT].led_states_count = sizeof(led_hollow_alrt) / sizeof(led_hollow_alrt[0]);
    led_hollow_conf[LED_PATTERN_ALERT].led_states = (led_pattern_state_t *)led_hollow_alrt;

    led_hollow_conf[LED_PATTERN_SETUP].led_states_count = sizeof(led_hollow_setup_mode) / sizeof(led_hollow_setup_mode[0]);
    led_hollow_conf[LED_PATTERN_SETUP].led_states = (led_pattern_state_t *)led_hollow_setup_mode;

    led_hollow_conf[LED_PATTERN_OFF].led_states_count = sizeof(led_hollow_off) / sizeof(led_hollow_off[0]);
    led_hollow_conf[LED_PATTERN_OFF].led_states = (led_pattern_state_t *)led_hollow_off;

    led_hollow_conf[LED_PATTERN_DO_NOT_DISTURB].led_states_count = sizeof(led_hollow_do_not_disturb) / sizeof(led_hollow_do_not_disturb[0]);
    led_hollow_conf[LED_PATTERN_DO_NOT_DISTURB].led_states = (led_pattern_state_t *)led_hollow_do_not_disturb;

    led_hollow_conf[LED_PATTERN_OTA].led_states_count = sizeof(led_hollow_ota) / sizeof(led_hollow_ota[0]);
    led_hollow_conf[LED_PATTERN_OTA].led_states = (led_pattern_state_t *)led_hollow_ota;

    is_init_done = true;
}
