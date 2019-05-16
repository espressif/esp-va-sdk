// Copyright 2018 Espressif Systems (Shanghai) PTE LTD
// All rights reserved.

#ifndef _PLAYBACK_CONTROLLER_H_
#define _PLAYBACK_CONTROLLER_H_

/* Specify the feature to be modified */
enum playback_controller_feature {
    /* Shuffle the songs */
    PLAYBACK_CONTROLLER_SHUFFLE = 1,
    /* Loop the song */
    PLAYBACK_CONTROLLER_LOOP,
    /* Repeat the song */
    PLAYBACK_CONTROLLER_REPEAT,
    /* Thumbs up the song */
    PLAYBACK_CONTROLLER_THUMBSUP,
    /* Thumbs down the song */
    PLAYBACK_CONTROLLER_THUMBSDOWN,
};

/* Specify the action for the specified feature */
enum playback_controller_action {
    /* Select the specified feature */
    PLAYBACK_CONTROLLER_SELECT = 1,
    /* Deselect the specified feature */
    PLAYBACK_CONTROLLER_DESELECT,
};

/** Notify play song
 *
 * This API may be called by the application if it wants to play the previous song
 * Alexa was playing.
 */
int playback_controller_notify_play();

/** Notify pause song
 *
 * This API may be called by the application if it wants to pause the song Alexa is playing.
 */
int playback_controller_notify_pause();

/** Notify next song
 *
 * This API may be called by the application if it wants to play the next song.
 */
int playback_controller_notify_next();

/** Notify next song
 *
 * This API may be called by the application if it wants to play the previous song.
 */
int playback_controller_notify_previous();

/** Notify skip forward
 *
 * This API may be called by the application if it wants to skip forward in the current playing song.
 */
int playback_controller_notify_skip_forward();

/** Notify skip backward
 *
 * This API may be called by the application if it wants to skip backward in the current playing song.
 */
int playback_controller_notify_skip_backward();

/** Notify feature modify
 *
 * This API may be called by the application if it wants to changea feature.
 *
 * \param[in] feature       Feature which is to be modified.
 * \param[in] action        Action to be performed on the specified feature.
 */
int playback_controller_notify_feature(enum playback_controller_feature feature, enum playback_controller_action action);

#endif /* _PLAYBACK_CONTROLLER_H_ */