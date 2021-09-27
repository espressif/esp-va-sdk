/*
 * ESPRESSIF MIT License
 *
 * Copyright (c) 2019 <ESPRESSIF SYSTEMS (SHANGHAI) CO., LTD>
 *
 * Permission is hereby granted for use on all ESPRESSIF SYSTEMS products, in which case,
 * it is free of charge, to any person obtaining a copy of this software and associated
 * documentation files (the "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the Software is furnished
 * to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include <esp_log.h>
#include <string.h>
#include <resampling.h>
#include <audio_board.h>
#include <esp_equalizer.h>
#include "media_hal_playback.h"
#include "esp_audio_mem.h"


static const char *TAG = "[media_hal_playback]";

#define POP_NOISE_FIX

#define MAX_PLAYBACK_REQUESTERS 2

#define CONVERT_BUF_SIZE 1024
#define BUF_SZ (CONVERT_BUF_SIZE * 12) /* Can handle 12x conv: 8k/1 --> 48k/2 */
static uint8_t *convert_buf;

static xSemaphoreHandle eq_mutex = NULL; /* To protect eq_handle */

/* Contains data or config relevant to a playback. */
typedef struct media_hal_playback {
    media_hal_playback_cfg_t cfg;
    audio_resample_config_t resample;
    void *eq_handle; /* equalizer handle */
    bool is_disabled;
} media_hal_playback_t;

static void *active_eq;
static media_hal_playback_t *media_hal_requesters[MAX_PLAYBACK_REQUESTERS];
static bool first_sound_flag = false;

static int default_write_callback(int port_num, void *buf, size_t len, int src_bps, int dst_bps)
{
    unsigned int sent_len = 0;
#ifdef CONFIG_HALF_DUPLEX_I2S_MODE
    if(i2s_mode != MODE_SPK) 
    {
        return sent_len;
    }
#endif    
    if (dst_bps == src_bps) {
        i2s_write((i2s_port_t) port_num, (char *) buf, len, &sent_len, portMAX_DELAY);
    } else {
        if (dst_bps > src_bps) {
            if (dst_bps % src_bps != 0) {
                ESP_LOGE(TAG, "destination bits need to be multiple of source bits");
                sent_len = -1;
                goto end;
            }
        } else {
            ESP_LOGE(TAG, "destination bits need to greater than and multiple of source bits");
            sent_len =  -1;
            goto end;
        }
        i2s_write_expand((i2s_port_t) port_num, (char *) buf, len, src_bps, dst_bps, &sent_len, portMAX_DELAY);
    }
end:    
    return sent_len;
}

static int default_equalizer_callback(char *buffer, int len, int sample_rate, int channels)
{
    int ret = 0;
    xSemaphoreTake(eq_mutex, portMAX_DELAY);
    if (__builtin_expect(!!active_eq, true)) { /* This could rarely be not set at this point. Recheck with mutex taken. */
        ret = esp_equalizer_process(active_eq, (unsigned char *) buffer, len, sample_rate, channels);
    }
    xSemaphoreGive(eq_mutex);
    return ret;
}

esp_err_t media_hal_equalizer_set_band_vals(const int8_t *gain_vals)
{
    for (int i = 0; i < MAX_PLAYBACK_REQUESTERS; i++) {
        if (!media_hal_requesters[i]) {
            break;
        }
        media_hal_playback_cfg_t *cfg = &media_hal_requesters[i]->cfg;
        void *eq_handle = media_hal_requesters[i]->eq_handle;

        if (__builtin_expect(cfg->equalizer_callback != default_equalizer_callback, false)) {
            ESP_LOGW(TAG, "Custom EQ callback was provided. Ignoring gain set.");
        }
        xSemaphoreTake(eq_mutex, portMAX_DELAY);
        if (!eq_handle) {
            ESP_LOGW(TAG, "Can't set gain values. Equalizer is not enabled");
        } else {
            for (int i = 0; i < MEDIA_HAL_EQ_BANDS; i++) { /* 10 bands */
                /* For two channels */
                esp_equalizer_set_band_value(eq_handle, gain_vals[i], i, 0);
                esp_equalizer_set_band_value(eq_handle, gain_vals[i], i, 1);
            }
        }
        xSemaphoreGive(eq_mutex);
    }
    return ESP_OK;
}

esp_err_t media_hal_enable_equalizer()
{
    if (!eq_mutex) {
        eq_mutex = xSemaphoreCreateMutex();
        if (!eq_mutex) {
            ESP_LOGE(TAG, "eq_mutex initialization failed");
            return ESP_FAIL;
        }
    }

    int i = 0;
    for (; i < MAX_PLAYBACK_REQUESTERS; i++) {
        if (!media_hal_requesters[i]) {
            break;
        }
        media_hal_playback_cfg_t *cfg = &media_hal_requesters[i]->cfg;
        if (cfg->equalizer_callback == NULL) {
            /* User didn't provide eq callback! Initialize with default one. */
            cfg->equalizer_callback = default_equalizer_callback;
        }
        if (__builtin_expect(cfg->equalizer_callback != default_equalizer_callback, false)) {
            ESP_LOGW(TAG, "Custom EQ callback was provided. Ignoring enable_equalizer.");
        } else {
            xSemaphoreTake(eq_mutex, portMAX_DELAY);
            if (!media_hal_requesters[i]->eq_handle) {
                media_hal_requesters[i]->eq_handle = esp_equalizer_init(cfg->channels, cfg->sample_rate, MEDIA_HAL_EQ_BANDS /* number of bands */, true);
            }
            if (!media_hal_requesters[i]->eq_handle) {
                ESP_LOGE(TAG, "esp_equalizer_init failed index = %d", i);
            }
            xSemaphoreGive(eq_mutex);
        }
    }

    if (i == 0) {
        return ESP_FAIL;
    }
    return ESP_OK;
}

void media_hal_disable_equalizer()
{
    for (int i = 0; i < MAX_PLAYBACK_REQUESTERS; i++) {
        if (!media_hal_requesters[i]) {
            break;
        }
        media_hal_playback_cfg_t *cfg = &media_hal_requesters[i]->cfg;

        if (__builtin_expect(cfg->equalizer_callback != default_equalizer_callback, false)) {
            ESP_LOGW(TAG, "Custom EQ callback was provided. Ignoring disable_equalizer.");
        }
        xSemaphoreTake(eq_mutex, portMAX_DELAY);
        if (!media_hal_requesters[i]->eq_handle) {
            ESP_LOGW(TAG, "EQ not initialized yet");
        }
        esp_equalizer_deinit(media_hal_requesters[i]->eq_handle);
        media_hal_requesters[i]->eq_handle = NULL;
        cfg->equalizer_callback = NULL;
        xSemaphoreGive(eq_mutex);
    }
}

esp_err_t media_hal_enable_playback(void *playback_handle)
{
    if (!playback_handle) {
        return ESP_FAIL;
    }
    media_hal_playback_t *pb_handle = (media_hal_playback_t *) playback_handle;
    for (int i = 0; i < MAX_PLAYBACK_REQUESTERS; i++) {
        if (!media_hal_requesters[i]) {
            break;
        }
        if (media_hal_requesters[i] == pb_handle) {
            pb_handle->is_disabled = false;
            return ESP_OK;
        }
    }
    return ESP_FAIL;
}

esp_err_t media_hal_disable_playback(void *playback_handle)
{
    if (!playback_handle) {
        return ESP_FAIL;
    }
    media_hal_playback_t *pb_handle = (media_hal_playback_t *) playback_handle;
    for (int i = 0; i < MAX_PLAYBACK_REQUESTERS; i++) {
        if (!media_hal_requesters[i]) {
            break;
        }
        if (media_hal_requesters[i] == pb_handle) {
            pb_handle->is_disabled = true;
            return ESP_OK;
        }
    }
    return ESP_FAIL;
}

void *media_hal_init_playback(media_hal_playback_cfg_t *cfg)
{
    if (!convert_buf) {
        convert_buf = esp_audio_mem_calloc(1, BUF_SZ);
        if (!convert_buf) {
            ESP_LOGE(TAG, "convert_buf allocation failed");
            return NULL;
        }
    }

    if (!eq_mutex) {
        eq_mutex = xSemaphoreCreateMutex();
        if (!eq_mutex) {
            ESP_LOGE(TAG, "eq_mutex initialization failed");
        }
    }

    /* Iterate through existing configs and find a spot */
    int i = 0;
    for (; i < MAX_PLAYBACK_REQUESTERS; i++) {
        if (!media_hal_requesters[i]) {
            break;
        }
    }
    if (i > MAX_PLAYBACK_REQUESTERS) {
        ESP_LOGW(TAG, "configs limit exhausted! Please set this to larger number");
        return NULL;
    }
    /* Allocate media_hal_playback structure. Just 0 for now */
    media_hal_requesters[i] = esp_audio_mem_calloc(1, sizeof (media_hal_playback_t));

    memcpy(&media_hal_requesters[i]->cfg, cfg, sizeof (media_hal_playback_cfg_t));
    if (media_hal_requesters[i]->cfg.write_callback == NULL) {
        media_hal_requesters[i]->cfg.write_callback = default_write_callback;
    }
    return media_hal_requesters[i];
}

int media_hal_playback_play(media_hal_playback_t *playback, media_hal_audio_info_t *audio_info, void *buf, int len)
{
    media_hal_playback_cfg_t *cfg = &playback->cfg;
    audio_resample_config_t *resample = &playback->resample; /* We need these to be separate */
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

    if ((audio_info->channels == 1) && (cfg->channels == 2))  {
        /* If mono recording, we need to up-sample, so need half the buffer empty, also uint16_t data*/
        convert_block_len = CONVERT_BUF_SIZE / 4;
    } else if ((audio_info->channels == 2) && (cfg->channels == 1)) {
        /* If down channel, we won't need additional buffer space but data is uint16_t*/
        convert_block_len = CONVERT_BUF_SIZE / 2;
    } else {
        /* If channels remain same, we need additional buffer space if upsampling*/
        convert_block_len = CONVERT_BUF_SIZE / 4;
    }
    
    while (len) {
        current_convert_block_len = (convert_block_len > len) ? len : convert_block_len;
        if (current_convert_block_len & 1) {
            printf("%s: Odd bytes in up sampling data, this should be backed up\n", TAG);
        }
        
        if (((audio_info->channels == 2) && (cfg->channels == 2)) || ((audio_info->channels == 1) && (cfg->channels == 1))) {
            conv_len = audio_resample((short *) ((char *) buf + send_offset), (short *) convert_buf, audio_info->sample_rate,
                                                 cfg->sample_rate, current_convert_block_len / 2, BUF_SZ, audio_info->channels, resample);
        } else if ((audio_info->channels == 1) && (cfg->channels == 2))  {
            conv_len = audio_resample_up_channel((short *) ((char *) buf + send_offset), (short *) convert_buf, audio_info->sample_rate,
                                                 cfg->sample_rate, current_convert_block_len / 2, BUF_SZ, resample);
        } else if ((audio_info->channels == 2) && (cfg->channels == 1)) {
            conv_len = audio_resample_down_channel((short *) ((char *) buf + send_offset), (short *) convert_buf, audio_info->sample_rate,
                                                 cfg->sample_rate, current_convert_block_len / 2, BUF_SZ, 0, resample);
        }

        len -= current_convert_block_len;
        /* The reason send_offset and send_len are different is because we could be converting from 24K to 16K */

        send_offset += current_convert_block_len;
        if (first_sound_flag == false) {
            //i2s_set_tx_buffer_flag();
            first_sound_flag = true;
        }

        if (cfg->equalizer_callback) {
            active_eq = playback->eq_handle;
            cfg->equalizer_callback((void *) convert_buf, conv_len * 2, cfg->sample_rate, cfg->channels);
        }
        
        cfg->write_callback((int) cfg->i2s_port_num, (void *) convert_buf, conv_len * 2,
                                        audio_info->bits_per_sample, cfg->bits_per_sample);
    }
    return sent_len;
}

int media_hal_playback(media_hal_audio_info_t *audio_info, void *buf, int len)
{
    //printf("%s: [resample-cb] %d spiram %d\n", TAG, heap_caps_get_free_size_sram(), heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
    //memset(convert_buf, 0, BUF_SZ);

    int sent_len = 0;
    for (int i = 0; i < MAX_PLAYBACK_REQUESTERS; i++) {
        if (!media_hal_requesters[i]) {
            break;
        }
        if (!media_hal_requesters[i]->is_disabled) {
            sent_len += media_hal_playback_play(media_hal_requesters[i], audio_info, buf, len);
        }
    }
    return sent_len;
}
