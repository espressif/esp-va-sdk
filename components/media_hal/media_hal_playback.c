#include <esp_log.h>
#include <string.h>
#include <resampling.h>
#include <audio_board.h>
#include <esp_equalizer.h>
#include "media_hal_playback.h"

#define POP_NOISE_FIX

#define CONVERT_BUF_SIZE 1024
#define BUF_SZ (CONVERT_BUF_SIZE * 2)
static uint8_t convert_buf[BUF_SZ];

static void *eq_handle = NULL; /* equalizer handle */
static xSemaphoreHandle eq_mutex = NULL; /* To protect eq_handle */

static media_hal_playback_cfg_t playback_cfg;

static const char *TAG = "[media_hal_playback]";
static bool first_sound_flag = false;

static int default_write_callback(int port_num, void *buf, size_t len, int src_bps, int dst_pbs)
{
    unsigned int sent_len = 0;
    if (dst_pbs == src_bps) {
        i2s_write((i2s_port_t) port_num, (char *) buf, len, &sent_len, portMAX_DELAY);
    } else {
        if (dst_pbs > src_bps) {
            if (dst_pbs % src_bps != 0) {
                ESP_LOGE(TAG, "destination bits need to be multiple of source bits");
                return -1;
            }
        } else {
            ESP_LOGE(TAG, "destination bits need to greater then and multiple of source bits");
            return -1;
        }
        i2s_write_expand((i2s_port_t) port_num, (char *) buf, len, src_bps, dst_pbs, &sent_len, portMAX_DELAY);
    }
    return sent_len;
}

static int default_equalizer_callback(char *buffer, int len, int sample_rate, int channels)
{
    int ret = 0;
    xSemaphoreTake(eq_mutex, portMAX_DELAY);
    if (__builtin_expect(eq_handle, true)) { /* This could rarely be not set at this point. Recheck with mutex taken. */
        ret = esp_equalizer_process(eq_handle, (unsigned char *) buffer, len, sample_rate, channels);
    }
    xSemaphoreGive(eq_mutex);
    return ret;
}

esp_err_t media_hal_equalizer_set_band_vals(const int8_t *gain_vals)
{
    if (__builtin_expect(playback_cfg.equalizer_callback != default_equalizer_callback, false)) {
        ESP_LOGW(TAG, "Custom EQ callback was provided. Ignoring gain set.");
        return ESP_FAIL;
    }
    xSemaphoreTake(eq_mutex, portMAX_DELAY);
    if (!eq_handle) {
        ESP_LOGW(TAG, "Can't set gain values. Equalizer is not enabled");
        xSemaphoreGive(eq_mutex);
        return ESP_FAIL;
    }
    for (int i = 0; i < MEDIA_HAL_EQ_BANDS; i++) { /* 10 bands */
        /* For two channels */
        esp_equalizer_set_band_value(eq_handle, gain_vals[i], i, 0);
        esp_equalizer_set_band_value(eq_handle, gain_vals[i], i, 1);
    }
    xSemaphoreGive(eq_mutex);
    return ESP_OK;
}

esp_err_t media_hal_enable_equalizer()
{
    if (playback_cfg.equalizer_callback == NULL) {
        /* User didn't provide eq callback! Initialize with default one. */
        playback_cfg.equalizer_callback = default_equalizer_callback;
    }
    if (__builtin_expect(playback_cfg.equalizer_callback != default_equalizer_callback, false)) {
        ESP_LOGW(TAG, "Custom EQ callback was provided. Ignoring enable_equalizer.");
        return ESP_FAIL;
    }
    if (!eq_mutex) {
        eq_mutex = xSemaphoreCreateMutex();
        if (!eq_mutex) {
            ESP_LOGE(TAG, "eq_mutex initialization failed");
            return ESP_FAIL;
        }
    }

    esp_err_t ret = ESP_OK;
    xSemaphoreTake(eq_mutex, portMAX_DELAY);
    if (!eq_handle) {
        eq_handle = esp_equalizer_init(playback_cfg.channels, playback_cfg.sample_rate, MEDIA_HAL_EQ_BANDS /* number of bands */, true);
    }
    if (!eq_handle) {
        ESP_LOGE(TAG, "esp_equalizer_init failed.");
        xSemaphoreGive(eq_mutex);
        ret = ESP_FAIL;
    }
    xSemaphoreGive(eq_mutex);
    return ret;
}

void media_hal_disable_equalizer()
{
    if (__builtin_expect(playback_cfg.equalizer_callback != default_equalizer_callback, false)) {
        ESP_LOGW(TAG, "Custom EQ callback was provided. Ignoring disable_equalizer.");
        return;
    }
    xSemaphoreTake(eq_mutex, portMAX_DELAY);
    if (!eq_handle) {
        ESP_LOGW(TAG, "EQ not initialized yet");
    }
    esp_equalizer_deinit(eq_handle);
    eq_handle = NULL;
    playback_cfg.equalizer_callback = NULL;
    xSemaphoreGive(eq_mutex);
}

void media_hal_init_playback(media_hal_playback_cfg_t *cfg)
{
    if (!eq_mutex) {
        eq_mutex = xSemaphoreCreateMutex();
        if (!eq_mutex) {
            ESP_LOGE(TAG, "eq_mutex initialization failed");
        }
    }
    memcpy(&playback_cfg, cfg, sizeof (media_hal_playback_cfg_t));
    if (playback_cfg.write_callback == NULL) {
        playback_cfg.write_callback = default_write_callback;
    }
}

int media_hal_playback(media_hal_audio_info_t *audio_info, void *buf, int len)
{
    //printf("%s: [resample-cb] %d spiram %d\n", TAG, heap_caps_get_free_size_sram(), heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
    memset(convert_buf, 0, BUF_SZ);
    static audio_resample_config_t resample = {0};
    int current_convert_block_len;
    int convert_block_len = 0;
    int send_offset = 0;
    size_t sent_len = 0;
    int conv_len = 0;

#ifdef POP_NOISE_FIX
    /**
     * Just check if first sample is zero and replace it with -1, a very small value.
     * pop-pop noise on es8388 codec devices can be fixed with this hack.
     */
    uint32_t *x = (uint32_t *) buf;
    if (__builtin_expect(*x == 0, false)) {
        *x = 0xffffffff;
    }
#endif

    if (audio_info->channels == 1) {
        /* If mono recording, we need to up-sample, so need half the buffer empty, also uint16_t data*/
        convert_block_len = CONVERT_BUF_SIZE / 4;
    } else {
        /* If stereo, we won't need additional buffer space but data is uint16_t*/
        convert_block_len = CONVERT_BUF_SIZE / 2;
    }

    while (len) {
        current_convert_block_len = (convert_block_len > len) ? len : convert_block_len;
        if (current_convert_block_len & 1) {
            printf("%s: Odd bytes in up sampling data, this should be backed up\n", TAG);
        }
        conv_len = audio_resample((short *) ((char *) buf + send_offset), (short *) convert_buf, audio_info->sample_rate,
                            playback_cfg.sample_rate, current_convert_block_len / 2, BUF_SZ, audio_info->channels, &resample);
        if (audio_info->channels == 1) {
            conv_len = audio_resample_up_channel((short *) convert_buf, (short *) convert_buf,
                            playback_cfg.sample_rate, playback_cfg.sample_rate, conv_len, BUF_SZ, &resample);
        }

        len -= current_convert_block_len;
        /* The reason send_offset and send_len are different is because we could be converting from 24K to 16K */

        send_offset += current_convert_block_len;
        if (first_sound_flag == false) {
            //i2s_set_tx_buffer_flag();
            first_sound_flag = true;
        }

        if (playback_cfg.equalizer_callback) {
            playback_cfg.equalizer_callback((void *) convert_buf, conv_len * 2, playback_cfg.sample_rate, playback_cfg.channels);
        }
        playback_cfg.write_callback((int) playback_cfg.i2s_port_num, (void *) convert_buf, conv_len * 2,
                                    audio_info->bits_per_sample, playback_cfg.bits_per_sample);
    }
    return sent_len;
}
