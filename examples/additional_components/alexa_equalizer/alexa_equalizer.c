// Copyright 2018 Espressif Systems (Shanghai) PTE LTD
// All rights reserved.

#include <string.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>
#include <str_utils.h>
#include <alexa_equalizer_controller.h>

#include "alexa_equalizer.h"
#include <media_hal_playback.h>

static const char *TAG = "[alexa_equalizer]";
static int8_t gain_vals[MEDIA_HAL_EQ_BANDS];

void alexa_equalizer_set_band_vals(int bass, int midrange, int treble)
{
    /**
     * Problem that is side effect of equalizer is that the amplitutes frequently cross 16 bit input values
     * These gets clipped as our output is also 16 bit and flat sequences and sound that sounds busted.
     * !!!Only if we could have 32 bit value inputs for all the audio codecs (DAC) and this problem won't be there!!!
     * It is better to take some other route to somewhat compensate this.
     * We make overall volume bit less. (by 3dB here (6 is the max of Alexa asks us.))
     * So, the range becomes [-9, 3] for us instead of [-6, 6]
     * Please note again that this just decrases volume and not equalizer effects.
     *      We anyway are playing normally when EQ is off.
     * Do experiment with these values in 10 bands.
     */
#define DECREASE_VOL (ALEXA_EQUALIZER_BAND_LEVEL_MAX / 2)

    /**
     * Prepare band values array to be set via `media_hal_init_eq_gain_vals`.
     * This mapping from Alexa's range of [`ALEXA_EQUALIZER_BAND_LEVEL_MIN`, `ALEXA_EQUALIZER_BAND_LEVEL_MAX`] to our custom
     *      could be tweaked as per our need.
     */
    ESP_LOGI(TAG, "Setting base: %d, midrange: %d, treble: %d", bass, midrange, treble);
    gain_vals[0] = bass - DECREASE_VOL;
    gain_vals[1] = bass - DECREASE_VOL;
    gain_vals[2] = bass - DECREASE_VOL;
    gain_vals[3] = midrange - DECREASE_VOL;
    gain_vals[4] = midrange - DECREASE_VOL;
    gain_vals[5] = midrange - DECREASE_VOL;
    gain_vals[6] = midrange - DECREASE_VOL;
    gain_vals[7] = treble - DECREASE_VOL;
    gain_vals[8] = treble - DECREASE_VOL;
    gain_vals[9] = treble - DECREASE_VOL;

    /* Request media hal to change equalizer gain values. */
    media_hal_equalizer_set_band_vals((const int8_t *) gain_vals);
}

void alexa_equalizer_set_mode(enum alexa_equalizer_controller_mode mode, int *bass, int *midrange, int *treble)
{
    /**
     * Amazon doesn't dictate what different values should be for different modes.
     * Let's set some values for now.
     * Make sure these values are within the limits.[`ALEXA_EQUALIZER_BAND_LEVEL_MIN`, `ALEXA_EQUALIZER_BAND_LEVEL_MAX`]
     */
    switch (mode) {
        case ALEXA_EQUALIZER_CONTROLLER_MOVIE:
            *bass = 2;
            *midrange = 2;
            *treble = 2;
            break;

        case ALEXA_EQUALIZER_CONTROLLER_NIGHT:
            *bass = 1;
            *midrange = 1;
            *treble = 1;
            break;

        default:
            *bass = 0;
            *midrange = 0;
            *treble = 0;
            break;
    }
    alexa_equalizer_set_band_vals(*bass, *midrange, *treble);
}

int alexa_equalizer_init()
{
    int bass, midrange, treble;
    enum alexa_equalizer_controller_mode current_mode = -1;

    /**
     * This is all the brains which does equalizations.
     * If not enabled, audio will play normal even though we keep getting calls to `alexa_equalizer_set_mode` or `alexa_equalizer_set_band_vals`
     * Please take a look at `media_hal_playback.h` for more info on how equalizer works.
     */
    media_hal_enable_equalizer();

    /* Set initial mode */
    alexa_equalizer_set_mode(current_mode, &bass, &midrange, &treble);

    /**
     * Initialize alexa equalizer controller with `alexa_equalizer_set_mode` and `alexa_equalizer_set_band_vals` callbacks.
     * Alexa will call this functions to alter band levels or to set mode whenever it is asked to.
     */
    alexa_equalizer_controller_init(current_mode, bass, midrange, treble, ALEXA_EQUALIZER_BAND_LEVEL_MIN, ALEXA_EQUALIZER_BAND_LEVEL_MAX,
                                    alexa_equalizer_set_mode, alexa_equalizer_set_band_vals);

    // If the equalizer is changed using the on-device buttons.
    // equalizer_controller_notify_level_change(EQUALIZER_CONTROLLER_NIGHT, 1, 1, 1);

    return 0;
}
