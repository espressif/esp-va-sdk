// Copyright 2018 Espressif Systems (Shanghai) PTE LTD
// All rights reserved.

#pragma once

#include <media_hal_playback.h>

typedef int (*read_cb_t)(void *cb_data, void *data, int len, unsigned int wait);
typedef void (*wakeup_reader_cb_t)(void *cb_data);

typedef struct {
    uint32_t samples_cnt;
    read_cb_t read_cb;
    wakeup_reader_cb_t wakeup_reader_cb;
    void *cb_data;
    media_hal_audio_info_t audio_info;
} sys_playback_requester_t;

/**
 * @brief   Put a `requester` in ducked mode
 *
 * If downmixing is supported, the ducked audio will be played in background with main audio up-front.
 */
int sys_playback_put_ducked(sys_playback_requester_t *requester);

/**
 * @brief   Remove `requester` from ducked mode
 *
 * If requester doesn't match with current duck, it will return.
 */
int sys_playback_remove_ducked(sys_playback_requester_t *requester);

/** Configuration for playback stream
 *  To be set by the application
 *  If the application does not set these values, then the default values are taken
 */
typedef struct {
    size_t stack_size;  //Default task stack size is 5000
    int task_priority;  //Default priority is 5
    size_t buf_size;    //Default buffer size is 512 bytes
    bool downmix_support;   //Default is disabled
} sys_playback_config_t;

/* If downmixing is supported. */
bool sys_playback_downmix_supported();

/**
 * @brief Initialize sys_playback with given config.
 *
 * Config is optional. If not provided, sys_playback will init without downmix support etc.
 */
int sys_playback_init(sys_playback_config_t *cfg);

/**
 * @brief Aquire sys_playback.
 *
 * Currently playing playback will be removed and `requester` will be put up-front to play from.
 */
int sys_playback_acquire(sys_playback_requester_t *requester);

/**
 * @brief Release current playback.
 */
int sys_playback_release(void);

/**
 * @brief Play a tone.
 *
 * This is fairly similar to `sys_playback_acquire` with a difference that it has highest precedence over all.
 * This doesn't replace current playing `requester`. It will resumed just after tone is done playing.
 */
int sys_playback_play_tone(sys_playback_requester_t *tone);

/**
 * @brief Get offset in milliseconds of registered `requester`.
 */
uint32_t sys_playback_get_current_offset(sys_playback_requester_t *requester);
