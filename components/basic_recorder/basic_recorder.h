// Copyright 2018 Espressif Systems (Shanghai) PTE LTD
// All rights reserved.

#pragma once

#include <audio_codec.h>
#include <abstract_rb.h>


/**
 * @brief Basic recorder events.
 *
 * These events are received by `event_cb` registered by applications.
 */
typedef enum basic_recorder_event {
    RECORDER_EVENT_STARTED,
    RECORDER_EVENT_STOPPED,
    RECORDER_EVENT_FAILED,
    RECORDER_EVENT_DOWNLOAD_COMPLETE,
    RECORDER_EVENT_FETCH_ANCHOR,
    RECORDER_EVENT_MAX,
} basic_recorder_event_t;

/* Basic recorder handle */
typedef void *basic_recorder_handle_t;

typedef ssize_t (*read_func_cb_t)(void *read_cb_data, void *data, int len, unsigned int wait);
typedef ssize_t (*write_func_cb_t)(void *write_cb_data, void *data, int len, unsigned int wait);

typedef int (*event_func_cb_t)(void *event_cb_data, basic_recorder_event_t event);

/**
 * @brief Basic recorder config structure.
 *
 * Application must send pointer to this.
 */
typedef struct basic_recorder_config {
    void *data; /* Not used for now */
} basic_recorder_cfg_t;


/**
 * @brief `basic_recorder_record` config
 *
 * Must be provided to `basic_recorder_record`
 */
typedef struct basic_recorder_record_config {
    audio_codec_type_t encoder_type;
    event_func_cb_t event_cb;
    read_func_cb_t read_cb;
    write_func_cb_t write_cb;
    void *event_cb_data;
    void *read_cb_data;
    void *write_cb_data;
} basic_recorder_record_config_t;

/**
 * @brief Create a new instance of basic recorder.
 */
basic_recorder_handle_t basic_recorder_create(basic_recorder_cfg_t *basic_recorder_cfg);

/**
 * @brief Add an initialized instance of codec to basic_recorder.
 *
 * Codec instance must be created by application.
 */
esp_err_t basic_recorder_add_codec(basic_recorder_handle_t handle, audio_codec_type_t type, audio_codec_t *base);

/**
 * @brief Remove a codec instance from basic recorder.
 *
 * The codec must be destroyed by application.
 */
esp_err_t basic_recorder_remove_codec(basic_recorder_handle_t handle, audio_codec_type_t type);

/**
 * @brief Start basic recorder with given record config.
 *
 * @note If recorder is already running, it will be stopped and will start with provided config.
 */
esp_err_t basic_recorder_record(basic_recorder_handle_t handle, basic_recorder_record_config_t *record_config);

/**
 * @brief Trigger basic recorder stop.
 */
esp_err_t basic_recorder_stop(basic_recorder_handle_t handle);

/**
 * @brief Destroy the recorder.
 *
 * All the structures of basic recorder will be cleaned up and then handle will be freed. Making it unusable.
 */
void basic_recorder_destroy(basic_recorder_handle_t handle);
