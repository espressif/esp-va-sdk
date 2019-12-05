// Copyright 2018 Espressif Systems (Shanghai) PTE LTD
// All rights reserved.

#ifndef _AUDIO_PLAYER_H_
#define _AUDIO_PLAYER_H_

#include <stdint.h>
#include <esp_err.h>

/** Used by the application to specify the play behaviour */
enum play_behaviour {
    /** Replace all the songs being played in the audio_player and play the current song */
    REPLACE_ALL,
    /** Enqueue the current song to the queue of the audio_player */
    ENQUEUE,
    /** Replace all te enqueued songs and play this song next */
    REPLACE_ENQUEUED,
    /** Used for internal purposes */
    INVALID_BEHAVIOUR,
};

/** Used by the application to specify the queue behaviour */
enum clear_behaviour {
    /** Clear the queue of the audio_player */
    CLEAR_ALL,
    /** Clear all the enqueued songs of the audio_player */
    CLEAR_ENQUEUED,
    /** Used for internal purposes */
    INVALID_CLEAR_BEHAVIOUR,
};

/** Used by to audio_player to notify the application via the callback */
enum event_type {
    /** Song started playing */
    AP_NOTIFY_PB_STARTED,
    /** Song downloading done */
    AP_NOTIFY_PB_DOWNLOAD_FINISHED,
    /** Song completed */
    AP_NOTIFY_PB_FINISHED,
    /** Song interrupted and stopped */
    AP_NOTIFY_PB_STOPPED,
    /** Song interrupted and paused */
    AP_NOTIFY_PB_PAUSED,
    /** Song resumed */
    AP_NOTIFY_PB_RESUMED,
    /** The queue is cleared */
    AP_NOTIFY_CLEAR_QUEUE,
    /** Song playing failed */
    AP_NOTIFY_ERROR,
    /** The specified interval has elapsed */
    AP_NOTIFY_INTERVAL_ELAPSED,
    /** The specified delay has elapsed */
    AP_NOTIFY_DELAY_ELAPSED,
};

/** Add a Song to be played by the audio_player
 *
 * \param[in] url               url of the song to be played
 * \param[in] token             token of the song to be played
 * \param[in] offset_in_ms      offset from which the song is to be played in milliseconds
 * \param[in] behaviour         specify the play_behaviour to play right now, play next or play after all the enqueued songs have played
 * \param[in] time_delay_elapsed    time in milliseconds after which a callback is given with AP_NOTIFY_DELAY_ELAPSED from the start of the stream
 * \param[in] time_interval_elapsed    time in milliseconds after which a callback is given with AP_NOTIFY_INTERVAL_ELAPSED every time_interval_elapsed milliseconds
 * \param[in] notify_cb         callback function which will be called to notify the application about the state of the audio_player
 *
 */
void audio_player_play(char *url, char *token, int offset_in_ms, enum play_behaviour behaviour, int time_delay_elapsed, int time_interval_elapsed, void (*notify_cb)(const char *token, uint32_t offset, enum event_type type));

/** Stop the song currently being played by the audio_player
 *
 * Use this just to stop the song being played from the calling API
 *
 */
void audio_player_stop(void (*notify_cb)(const char *token, uint32_t offset, enum event_type type));

/** Clear the song queue of the audio_player
 *
 * \param[in] behaviour         specify the clear_behaviour to be done with the queue of the audio_player
 *
 */
void audio_player_clear_queue(enum clear_behaviour behaviour);

/**
 * @brief Register clear queue callback
 *
 * @param callback clear queue callback function to register
 *
 * @return     int, 0--success, others--fail
 */
esp_err_t audio_player_register_clear_queue_cb(int (*callback) (void));

/**
 * @brief Deregister clear queue callback
 *
 * @param callback clear queue callback function to deregister
 *
 * @return     int, 0--success, others--fail
 */
esp_err_t audio_player_deregister_clear_queue_cb(int (*callback) (void));

#endif /* _AUDIO_PLAYER_H_ */
