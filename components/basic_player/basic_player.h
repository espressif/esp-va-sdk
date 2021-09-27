// Copyright 2018 Espressif Systems (Shanghai) PTE LTD
// All rights reserved.

#pragma once

#include <i2s_stream.h>
#include <http_playback_stream.h>
#include <audio_codec.h>
#include <abstract_rb.h>
#include "sys_playback.h"

#define DEFAULT_CODEC_OUTPUT_RB_SIZE (40 * 1024)
#define DEFAULT_HTTP_OUTPUT_RB_SIZE (100 * 1024)

#define DEFAULT_BASIC_PLAYER_CONFIG() {                     \
    .http_support = false,                                  \
    .codec_output_rb_size = DEFAULT_CODEC_OUTPUT_RB_SIZE,   \
    .http_output_rb_size = 0,                               \
    .rb_cfg = DEFAULT_RB_TYPE_BASIC_FUNC()                  \
}

/**
 * @brief Basic player events.
 *
 * These events are received by `event_cb` registered by applications.
 */
typedef enum basic_player_event {
    PLAYER_EVENT_STARTED,
    PLAYER_EVENT_STOPPED,
    PLAYER_EVENT_FAILED,
    PLAYER_EVENT_DOWNLOAD_COMPLETE,
    PLAYER_EVENT_FETCH_ANCHOR,
    PLAYER_EVENT_MAX,
} basic_player_event_t;

/* Basic player handle */
typedef void *basic_player_handle_t;

typedef ssize_t (*read_func_cb_t)(void *read_cb_data, void *data, int len, unsigned int wait);
typedef int (*event_func_cb_t)(void *event_cb_data, basic_player_event_t event);
typedef void (*read_len_func_cb_t)(void *read_cb_data, int len);

/**
 * @brief Basic player config structure.
 *
 * Application must send pointer to this.
 */
typedef struct basic_player_config {
    bool http_support;
    uint32_t codec_output_rb_size;
    uint32_t http_output_rb_size;
    abstract_rb_cfg_t rb_cfg;
} basic_player_cfg_t;

enum basic_player_play_method {
    PLAY_FROM_CB,
    PLAY_FROM_URL,
};

/**
 * @brief `basic_player_play` config
 *
 * Must be provided to `basic_player_play`
 */
typedef struct basic_player_play_config {
    enum basic_player_play_method play_method;
    union play_method_details {
        struct http {
            char *url;
            read_len_func_cb_t read_len_cb;
            void *read_cb_data;
        } http;
        struct callback {
            audio_codec_type_t decoder_type;
            read_func_cb_t read_cb;
            void *read_cb_data;
        } callback;
    } play_method_details;
    int offset_in_ms;
    event_func_cb_t event_cb;
    void *event_cb_data;
} basic_player_play_config_t;

/**
 * @brief Create a new instance of basic player.
 */
basic_player_handle_t basic_player_create(basic_player_cfg_t *basic_player_cfg);

/**
 * @brief Add an initialized instance of codec to basic_player.
 *
 * Codec instance must be created by application.
 */
esp_err_t basic_player_add_codec(basic_player_handle_t handle, audio_codec_type_t type, audio_codec_t *base);

/**
 * @brief Remove a codec instance from basic player.
 *
 * The codec must be destroyed by application.
 */
esp_err_t basic_player_remove_codec(basic_player_handle_t handle, audio_codec_type_t type);

/**
 * @brief Start basic player with given play config.
 *
 * @note If player is already running, it will be stopped and will start with provided config.
 */
esp_err_t basic_player_play(basic_player_handle_t handle, basic_player_play_config_t *play_config);

/**
 * @brief Trigger basic player stop.
 */
esp_err_t basic_player_stop(basic_player_handle_t handle);

/**
 * @brief Destroy the player.
 *
 * All the structures of basic player will be cleaned up and then handle will be freed. Making it unusable.
 */
void basic_player_destroy(basic_player_handle_t handle);

/**
 * @brief Get basic player playback requester.
 *
 * sys_playback uses this to keep reading decoded data and play.
 * The requester should be registered with sys_playback by upper layer.
 */
sys_playback_requester_t *basic_player_get_playback_requester(basic_player_handle_t handle);

/**
 * @brief Get current played offset in milliseconds.
 *
 * Basic players works with sys_playback to know current play offset.
 */
uint32_t basic_player_get_current_offset(basic_player_handle_t handle);

/**
 * @brief Get the number of bytes that can be played.
 *
 * Returns the current number of bytes in player's out rb.
 */
int basic_player_get_codec_output_rb_filled(basic_player_handle_t handle);

/**
 * @brief put an anchor to player's input ringbuffer.
 */
void basic_player_put_anchor(basic_player_handle_t handle, void *data, uint32_t datalen);

/**
 * @brief get an anchor from player's out rb.
 *
 * This is NULL when the basic_player is initialized with normal rb.
 */
void basic_player_get_anchor(basic_player_handle_t handle, int *offset, void *data, uint32_t datalen);
