// Copyright 2018 Espressif Systems (Shanghai) PTE LTD
// All rights reserved.

#ifndef _ALEXA_EQUALIZER_CONTROLLER_H_
#define _ALEXA_EQUALIZER_CONTROLLER_H_

/* The maximum and minimum level for each equalizer band which is supported. */
#define ALEXA_EQUALIZER_BAND_LEVEL_MIN -6
#define ALEXA_EQUALIZER_BAND_LEVEL_MAX 6

/* Supported equalizer modes. The application must implement these
 * modes i.e. the value of the bands (bass, midrange and treble) in these modes. */
enum alexa_equalizer_controller_mode {
    ALEXA_EQUALIZER_CONTROLLER_MOVIE,
    ALEXA_EQUALIZER_CONTROLLER_MUSIC,
    ALEXA_EQUALIZER_CONTROLLER_NIGHT,
    ALEXA_EQUALIZER_CONTROLLER_SPORT,
    ALEXA_EQUALIZER_CONTROLLER_TV,
    /* maximum modes */
    ALEXA_EQUALIZER_CONTROLLER_MODE_MAX,
};

/** Initialise the equalizer controller
 *
 * \param[in] mode              the default mode at the start
 * \param[in] bass              the default bass level at the start
 * \param[in] midrange          the default midrange level at the start
 * \param[in] treble            the default treble level at the start
 * \param[in] level_min         the minimum level of bands. Default is ALEXA_EQUALIZER_BAND_LEVEL_MIN
 * \param[in] level_max         the maximum level of bands. Default is ALEXA_EQUALIZER_BAND_LEVEL_MAX
 * \param[in] set_mode          callback function to set one of the supported equalizer modes. This is called by the SDK.
 *                              The bass, midrange and the treble values that are set should also be assigned to the out parameters.
 * \param[in] set_band_level    callback function to set the level of a band or bands. This is called by the SDK.
 *                              The the band levels according to the values must be set.
 * \return  0 on success, -1 otherwise
 *
 */
int alexa_equalizer_controller_init(enum alexa_equalizer_controller_mode mode, int bass, int midrange, int treble, int level_min, int level_max,
                                    void (*set_mode)(enum alexa_equalizer_controller_mode mode, int *bass, int *midrange, int *treble),
                                    void (*set_band_level)(int bass, int midrange, int treble));

/** Get current equalizer levels
 *
 * \param[out] mode     current equalizer mode. Default is EQUALIZER_CONTROLLER_MOVIE
 * \param[out] bass     current bass level. Default is 0
 * \param[out] midrange current midrange level. Default is 0
 * \param[out] treble   current treble level. Default is 0
 */
void alexa_equalizer_controller_get_current_levels(enum alexa_equalizer_controller_mode *mode, int *bass, int *midrange, int *treble);

/* Notify band level change
 * This API should be called by the application to notify Alexa that the
 * user has changed the equalizer band levels using the on-device buttons.
 *
 * \param[in] mode              the current mode which is set. If mode is unchanged, EQUALIZER_CONTROLLER_MODE_MAX should be set
 * \param[in] bass              the current bass level. If unchanged, use alexa_equalizer_controller_get_current_levels() to get current bass level and pass to this function
 * \param[in] midrange          the current midrange level. If unchanged, use alexa_equalizer_controller_get_current_levels() to get current bass level and pass to this function
 * \param[in] treble            the current treble level. If unchanged, use alexa_equalizer_controller_get_current_levels() to get current bass level and pass to this function
 */
void alexa_equalizer_controller_notify_level_change(enum alexa_equalizer_controller_mode mode, int bass, int midrange, int treble);

#endif /* _ALEXA_EQUALIZER_CONTROLLER_H_ */
