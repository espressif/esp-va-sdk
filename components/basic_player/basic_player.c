// Copyright 2018 Espressif Systems (Shanghai) PTE LTD
// All rights reserved.

#include <string.h>

#include <string.h>
#include <esp_audio_mem.h>
#include <esp_log.h>
#include <http_playback_stream.h>
#include <esp_err.h>
#include <abstract_rb_utils.h>
#include "basic_player.h"

#define HTTP_PLAYBACK_STREAM_STACK_SIZE (21 * 1024)

static const char *TAG = "[basic_player]";

struct basic_player {
    rb_handle_t codec_output_rb;
    rb_handle_t http_output_rb;
    sys_playback_requester_t requester;
    struct audio_codec_list {
        audio_codec_t *base;
    } codec[CODEC_TYPE_DEC_MAX]; /* We do not need encoder types */
    audio_codec_t *current_playing;
    http_playback_stream_t *http_stream;

    enum basic_player_play_method play_method;
    http_playback_stream_config_t hs_cfg;
    const char *http_url;
    read_func_cb_t codec_read_cb;
    event_func_cb_t player_event_cb;
    read_len_func_cb_t read_len_cb;
    void *codec_read_cb_data;
    void *player_event_cb_data;
    void *read_len_cb_data;

    SemaphoreHandle_t lock;

    bool stop_event_sent;
    bool is_http_stopped;
    bool is_codec_stopped;
    bool is_playing;
};

static ssize_t basic_player_http_read_cb(void *arg, void *data, int len, unsigned int wait);

sys_playback_requester_t *basic_player_get_playback_requester(basic_player_handle_t handle)
{
    if (handle == NULL) {
        ESP_LOGE(TAG, "Handle is null");
        return NULL;
    }
    struct basic_player *b = (struct basic_player *)handle;
    return &b->requester;
}

uint32_t basic_player_get_current_offset(basic_player_handle_t handle)
{
    if (handle == NULL) {
        ESP_LOGE(TAG, "Handle is null");
        return 0;
    }
    struct basic_player *b = (struct basic_player *)handle;
    return sys_playback_get_current_offset(&b->requester);
}

int basic_player_get_codec_output_rb_filled(basic_player_handle_t handle)
{
    if (handle == NULL) {
        ESP_LOGE(TAG, "Handle is null");
        return 0;
    }
    struct basic_player *b = (struct basic_player *)handle;
    return arb_get_filled(b->codec_output_rb);
}

void basic_player_put_anchor(basic_player_handle_t handle, void *data, uint32_t datalen)
{
    if (handle == NULL) {
        ESP_LOGE(TAG, "Handle is null");
        return;
    }
    struct basic_player *b = (struct basic_player *)handle;
    arb_utils_put_anchor_at_current(b->codec_output_rb, data, datalen);
}

void basic_player_get_anchor(basic_player_handle_t handle, int *offset, void *data, uint32_t datalen)
{
    if (handle == NULL) {
        ESP_LOGE(TAG, "Handle is null");
        return;
    }
    struct basic_player *b = (struct basic_player *)handle;
    arb_utils_get_anchor(b->codec_output_rb, offset, data, datalen);
}

static void basic_player_wait_for_stop_and_reset(void *arg)
{
    struct basic_player *b = (struct basic_player *)arg;

    int spin_counter = 0;
    while(!b->is_http_stopped) {
        if (spin_counter++ > 20) {
            ESP_LOGW(TAG, "Waiting for http_stream to stop line %d", __LINE__);
            spin_counter = 0;
        } else {
            ESP_LOGI(TAG, "Waiting for http_stream to stop line %d", __LINE__);
        }
        vTaskDelay(50 / portTICK_PERIOD_MS);
    }
    spin_counter = 0;
    while(!b->is_codec_stopped) {
        if (spin_counter++ > 20) {
            ESP_LOGW(TAG, "Waiting for decoder to stop line %d", __LINE__);
            spin_counter = 0;
        } else {
            ESP_LOGI(TAG, "Waiting for decoder to stop line %d", __LINE__);
        }
        vTaskDelay(50 / portTICK_PERIOD_MS);
    }

    if (b->play_method == PLAY_FROM_URL) {
        arb_reset(b->http_output_rb);
    }
    arb_reset(b->codec_output_rb);

    b->is_playing = false;

    /**
     * The event might get sent twice.
     * Need to make sure, by protection, that this never happens.
     */
    xSemaphoreTake(b->lock, portMAX_DELAY);
    if (!b->stop_event_sent) {
        b->player_event_cb(b->player_event_cb_data, PLAYER_EVENT_STOPPED);
        b->stop_event_sent = true;
    }
    xSemaphoreGive(b->lock);
}

esp_err_t basic_player_play(basic_player_handle_t handle, basic_player_play_config_t *play_config)
{
    if (handle == NULL) {
        ESP_LOGE(TAG, "Handle is null");
        return ESP_FAIL;
    }
    struct basic_player *b = (struct basic_player *)handle;

    basic_player_stop(handle);
    b->requester.samples_cnt = 0;
    b->player_event_cb = play_config->event_cb;
    b->player_event_cb_data = play_config->event_cb_data;
    b->play_method = play_config->play_method;

    b->is_http_stopped = true;
    b->is_codec_stopped = true;
    b->stop_event_sent = false;
    b->is_playing = true;

    arb_reset(b->codec_output_rb);

    if (b->play_method == PLAY_FROM_CB) {
        audio_codec_type_t codec_type = play_config->play_method_details.callback.decoder_type; // Add a check here.
        b->codec_read_cb = play_config->play_method_details.callback.read_cb; // Add a check here.
        b->codec_read_cb_data = play_config->play_method_details.callback.read_cb_data; // Add a check here.
        if (b->codec[codec_type].base) {
            audio_codec_set_offset(b->codec[codec_type].base, play_config->offset_in_ms);
            audio_codec_start(b->codec[codec_type].base);
            b->current_playing = b->codec[codec_type].base;
        } else {
            return ESP_FAIL;
        }
    } else if (b->play_method == PLAY_FROM_URL) {
        arb_reset(b->http_output_rb);
        b->codec_read_cb = basic_player_http_read_cb;
        b->codec_read_cb_data = (void *)b;
        b->read_len_cb = play_config->play_method_details.http.read_len_cb;
        b->read_len_cb_data = play_config->play_method_details.http.read_cb_data;
        b->hs_cfg.url = play_config->play_method_details.http.url;
        b->hs_cfg.offset_in_ms = play_config->offset_in_ms;
        http_playback_stream_set_config(b->http_stream, &b->hs_cfg);
        audio_stream_start(&b->http_stream->base);
    }
    return ESP_OK;
}


esp_err_t basic_player_stop(basic_player_handle_t handle)
{
    if (handle == NULL) {
        ESP_LOGE(TAG, "Handle is null");
        return ESP_FAIL;
    }
    struct basic_player *b = (struct basic_player *)handle;

    if (!b->is_playing) {
        return ESP_FAIL;
    }

    if (b->play_method == PLAY_FROM_URL) {
        audio_stream_stop(&b->http_stream->base);
        arb_abort(b->http_output_rb);
    }
    /**
     * Aborting the output buffer should be just fine
     * But if `codec_output_rb` is reset by external application before codec stops,
     * we get into trouble. A very next audio will not be able to play.
     * Do `audio_codec_stop` as well
     */
    audio_codec_stop(b->current_playing);
    arb_abort(b->codec_output_rb);

    basic_player_wait_for_stop_and_reset((void *)b);
    return ESP_OK;
}

static const char *basic_player_stream_get_codec_str(audio_codec_type_t type)
{
    switch (type) {
    case CODEC_TYPE_MP3_DECODER:
        return "mp3_decoder";
        break;
    case CODEC_TYPE_AAC_DECODER:
        return "aac_decoder";
        break;
    case CODEC_TYPE_OPUS_DECODER:
        return "opus_decoder";
        break;
    case CODEC_TYPE_WAV_DECODER:
        return "wav_decoder";
        break;
    case CODEC_TYPE_AMR_DECODER:
        return "amr_decoder";
        break;
    case CODEC_TYPE_MAX:
        return "invalid_decoder";
        break;
    default:
        break;
    }
    return "invalid_decoder";
}

static const char *basic_player_codec_get_event_str(audio_codec_event_t event)
{
    switch (event) {
    case CODEC_EVENT_STARTED:
        return "Codec Started";
        break;
    case CODEC_EVENT_STOPPED:
        return "Codec Stopped";
        break;
    case CODEC_EVENT_FAILED:
        return "Codec Failed";
        break;
    case CODEC_EVENT_PAUSED:
        return "Codec Paused";
        break;
    case CODEC_EVENT_DESTROYED:
        return "Codec Destroyed";
        break;
    case CODEC_EVENT_SET_FREQ:
        return "Codec Set Frequency";
        break;
    }
    return "Invalid";
}

static const char *basic_player_stream_get_event_str(audio_stream_event_t event)
{
    switch (event) {
    case STREAM_EVENT_STARTED:
        return "Started";
        break;
    case STREAM_EVENT_STOPPED:
        return "Stopped";
        break;
    case STREAM_EVENT_PAUSED:
        return "Paused";
        break;
    case STREAM_EVENT_DESTROYED:
        return "Destroyed";
        break;
    case STREAM_EVENT_CUSTOM_DATA:
        return "Custom-Data";
        break;
    case STREAM_EVENT_FAILED:
        return "Failed";
        break;
    default:
        break;
    }
    return "Invalid";
}

static audio_codec_type_t basic_player_codec_get_type_from_mime(http_hls_mime_type_t type)
{
    switch(type) {
        case MP3_URL:
            return CODEC_TYPE_MP3_DECODER;
            break;

        case AAC_URL:
            return CODEC_TYPE_AAC_DECODER;
            break;

        case OPUS_URL:
            return CODEC_TYPE_OPUS_DECODER;
            break;

        case WAV_URL:
            return CODEC_TYPE_WAV_DECODER;
            break;

        case AMR_URL:
        case AMR_WB_URL:
            return CODEC_TYPE_AMR_DECODER;

        default:
            return CODEC_TYPE_MAX;
            break;
    }
    return CODEC_TYPE_MAX;
}

static ssize_t basic_player_codec_read_cb(void *arg, void *data, int len, unsigned int wait)
{
    ssize_t ret;
    struct basic_player *b = (struct basic_player *)arg;
    do {
        ret = (*b->codec_read_cb)(b->codec_read_cb_data, data, len, wait);
    } while (ret == RB_FETCH_ANCHOR);
    return ret;
}

static ssize_t basic_player_codec_write_cb(void *arg, void *data, int len, unsigned int wait)
{
    ssize_t ret = len;
    struct basic_player *b = (struct basic_player *)arg;
    if (len > 0) {
        ret = arb_write(b->codec_output_rb, (uint8_t *)data, len, wait);
    }
    return ret;
}

static esp_err_t basic_player_codec_event_cb(void *arg, int event, void *data)
{
    struct basic_player *b = (struct basic_player *)arg;
    printf("%s: Codec event: %s\n", TAG, basic_player_codec_get_event_str(event));
    audio_codec_audio_info_t *info = NULL;

    switch(event) {
        case CODEC_EVENT_STARTED:
            b->is_codec_stopped = false;
            break;

        case CODEC_EVENT_STOPPED:
            b->is_codec_stopped = true;
            arb_signal_writer_finished(b->codec_output_rb);
            if (b->play_method == PLAY_FROM_URL) {
                /**
                 * If we for some reason get false `CODEC_EVENT_STOPPED` event, we won't be able to stop `http_stream`.
                 * This could make next playback or current stop non-deterministic.
                 *
                 * It doesn't hurt to abort the http buffer at this point anyway.
                 */
                arb_abort(b->http_output_rb);
            }
            break;

        case CODEC_EVENT_FAILED:
            b->is_codec_stopped = true;
            arb_signal_writer_finished(b->codec_output_rb);
            b->player_event_cb(b->player_event_cb_data, PLAYER_EVENT_FAILED);
            if (b->play_method == PLAY_FROM_URL) {
                arb_abort(b->http_output_rb);
            }
            break;

        case CODEC_EVENT_SET_FREQ:
            info = (audio_codec_audio_info_t *)data;
            ESP_LOGI(TAG, "Set Freq event: %d, %d, %d", info->sampling_freq, info->channels, info->bits);

            int sampling_freq_in_ms = info->sampling_freq / 1000;
            b->requester.audio_info.sample_rate = info->sampling_freq;
            b->requester.audio_info.channels = info->channels;
            b->requester.audio_info.bits_per_sample = 16;
            b->requester.samples_cnt = b->hs_cfg.offset_in_ms * sampling_freq_in_ms * 2 * info->channels;
            b->player_event_cb(b->player_event_cb_data, PLAYER_EVENT_STARTED);
            break;

        default:
            break;
    }
    return ESP_OK;
}

static ssize_t basic_player_http_read_cb(void *arg, void *data, int len, unsigned int wait)
{
    struct basic_player *b = (struct basic_player *)arg;
    return arb_read(b->http_output_rb, (uint8_t *)data, len, wait);
}

static ssize_t basic_player_http_write_cb(void *arg, void *data, int len, unsigned int wait)
{
    ssize_t ret = len;
    struct basic_player *b = (struct basic_player *)arg;
    if (len > 0) {
        if (arb_get_filled(b->http_output_rb) > (200 * 1024)) {
            vTaskDelay(pdMS_TO_TICKS(10));
        }
        ret = arb_write(b->http_output_rb, data, len, wait);
        if (b->read_len_cb) {
            b->read_len_cb(b->read_len_cb_data, len);
        }
    }
    return ret;
}

static esp_err_t basic_player_http_event_cb(void *arg, int event, void *data)
{
    esp_err_t ret = ESP_OK;
    struct basic_player *b = (struct basic_player *)arg;
    http_playback_stream_custom_data_t *custom_data = NULL;
    http_hls_mime_type_t content_type = NO_URL;
    audio_codec_type_t codec_type = CODEC_TYPE_MAX;
    int offset_in_ms = 0;
    printf("%s: HTTP writer stream event %s\n", TAG, basic_player_stream_get_event_str(event));

    switch (event) {
        case STREAM_EVENT_STARTED:
            b->is_http_stopped = false;
            break;

        case STREAM_EVENT_FAILED:
            b->is_playing = false;
            b->is_http_stopped = true;
            arb_signal_writer_finished(b->http_output_rb);
            b->player_event_cb(b->player_event_cb_data, PLAYER_EVENT_FAILED);
            break;

        case STREAM_EVENT_STOPPED:
            b->is_http_stopped = true;
            arb_signal_writer_finished(b->http_output_rb);
            b->player_event_cb(b->player_event_cb_data, PLAYER_EVENT_DOWNLOAD_COMPLETE);
            break;

        case STREAM_EVENT_CUSTOM_DATA:
            custom_data = (http_playback_stream_custom_data_t *)data;
            content_type = custom_data->content_type;
            codec_type = basic_player_codec_get_type_from_mime(content_type);
            offset_in_ms = custom_data->offset_in_ms;
            if (b->codec[codec_type].base) {
                audio_codec_set_offset(b->codec[codec_type].base, offset_in_ms);
                audio_codec_start(b->codec[codec_type].base);
                b->current_playing = b->codec[codec_type].base;
            } else {
                ESP_LOGE(TAG, "Unsupported decoder type %d", codec_type);
                ret = ESP_FAIL;
            }
            break;

        default:
            break;
    }
    return ret;
}

static int basic_player_i2s_read_cb(void *arg, void *data, int len, unsigned int wait)
{
    int ret;
    struct basic_player *b = (struct basic_player *)arg;
    ret = arb_read(b->codec_output_rb, (uint8_t *)data, len, wait);
    if (ret == RB_READER_UNBLOCK) {
        /* Just a wake-up */
    } else if (ret == RB_FETCH_ANCHOR) {
        /* Process the anchor */
        b->player_event_cb(b->player_event_cb_data, PLAYER_EVENT_FETCH_ANCHOR);
    } else if (ret < 0) {
        /* Stop the streams */
        basic_player_wait_for_stop_and_reset(arg);
    } else {
        /* Normal data */
    }
    return ret;
}

static void basic_player_i2s_wakeup_reader_cb(void *arg)
{
    struct basic_player *b = (struct basic_player *)arg;
    arb_wakeup_reader(b->codec_output_rb);
}

esp_err_t basic_player_add_codec(basic_player_handle_t handle, audio_codec_type_t type, audio_codec_t *base)
{
    if (handle == NULL) {
        ESP_LOGE(TAG, "Handle is null");
        return ESP_FAIL;
    }
    struct basic_player *b = (struct basic_player *)handle;
    if (b->codec[type].base) {
        ESP_LOGE(TAG, "audio_codec: %s already exists. Remove it first to change it.", basic_player_stream_get_codec_str(type));
        return ESP_FAIL;
    }

    b->codec[type].base = base;
    audio_io_fn_arg_t codec_read_fn = { .func = basic_player_codec_read_cb, .arg = b };
    audio_io_fn_arg_t codec_write_fn = { .func = basic_player_codec_write_cb, .arg = b };
    audio_event_fn_arg_t codec_event_fn = { .func = basic_player_codec_event_cb, .arg = b };

    if (audio_codec_init(b->codec[type].base, basic_player_stream_get_codec_str(type), &codec_read_fn, &codec_write_fn, &codec_event_fn) != 0) {
        ESP_LOGE(TAG, "Error initializing audio_codec: %s", basic_player_stream_get_codec_str(type));
        return ESP_FAIL;
    }
    return ESP_OK;
}

esp_err_t basic_player_remove_codec(basic_player_handle_t handle, audio_codec_type_t type)
{
    if (handle == NULL) {
        ESP_LOGE(TAG, "Handle is null");
        return ESP_FAIL;
    }
    struct basic_player *b = (struct basic_player *)handle;
    if (b->codec[type].base) {
        audio_codec_destroy(b->codec[type].base);
        b->codec[type].base = NULL;
        return ESP_OK;
    }
    return ESP_FAIL;
}

basic_player_handle_t basic_player_create(basic_player_cfg_t *basic_player_cfg)
{
    struct basic_player *b = (struct basic_player *) esp_audio_mem_calloc(1, sizeof(struct basic_player));
    if (!b) {
        ESP_LOGE(TAG, "Failed to create basic player");
        goto error;
    }
    memset(b, 0, sizeof(struct basic_player));
    if (basic_player_cfg == NULL) {
        ESP_LOGE(TAG, "basic_player config is NULL. Invalid parameter");
        goto error;
    }

    b->lock = xSemaphoreCreateMutex();
    if (!b->lock) {
        ESP_LOGE(TAG, "Couldn't create lock");
        goto error;
    }

    b->requester.read_cb = basic_player_i2s_read_cb;
    b->requester.wakeup_reader_cb = basic_player_i2s_wakeup_reader_cb;
    b->requester.cb_data = (void *)b;

    if (basic_player_cfg->codec_output_rb_size == 0) {
        ESP_LOGW(TAG, "No codec output rb size provided. Setting default to %d KB.", DEFAULT_CODEC_OUTPUT_RB_SIZE / 1024);
        basic_player_cfg->codec_output_rb_size = DEFAULT_CODEC_OUTPUT_RB_SIZE;
    }
    b->codec_output_rb = arb_init("codec_output_rb", basic_player_cfg->codec_output_rb_size, basic_player_cfg->rb_cfg);
    if (!b->codec_output_rb) {
        ESP_LOGE(TAG, "arb_init failed for codec_output of size: %d", basic_player_cfg->codec_output_rb_size);
        goto error;
    }

    if (basic_player_cfg->http_support) {
        audio_io_fn_arg_t http_stream_write_fn = { .func = basic_player_http_write_cb, .arg = b };
        audio_event_fn_arg_t http_stream_event_fn = { .func = basic_player_http_event_cb, .arg = b };

        if (basic_player_cfg->http_output_rb_size == 0) {
            ESP_LOGW(TAG, "No http output rb size provided. Setting default to %d KB.", DEFAULT_HTTP_OUTPUT_RB_SIZE / 1024);
            basic_player_cfg->http_output_rb_size = DEFAULT_HTTP_OUTPUT_RB_SIZE;
        }

        b->http_output_rb = arb_init("http_output_rb", basic_player_cfg->http_output_rb_size, basic_player_cfg->rb_cfg);
        if (!b->http_output_rb) {
            ESP_LOGE(TAG, "arb_init failed for http_output of size: %d", basic_player_cfg->http_output_rb_size);
            goto error;
        }

        b->http_stream = http_playback_stream_create_reader(&b->hs_cfg);
        if (!b->http_stream) {
            ESP_LOGE(TAG, "http_stream_create failed");
            goto error;
        }
        http_playback_stream_set_stack_size(b->http_stream, HTTP_PLAYBACK_STREAM_STACK_SIZE);
        if (audio_stream_init(&b->http_stream->base, "http_stream", &http_stream_write_fn, &http_stream_event_fn) != 0) {
            ESP_LOGE(TAG, "Error initializing audio_stream for http");
            goto error;
        }
    }
    return (basic_player_handle_t)b;

error:
    basic_player_destroy((basic_player_handle_t)b);
    return NULL;
}

void basic_player_destroy(basic_player_handle_t handle)
{
    if (handle == NULL) {
        ESP_LOGE(TAG, "Handle is null");
        return;
    }
    struct basic_player *b = (struct basic_player *)handle;
    if (b->is_playing) {
        basic_player_stop((basic_player_handle_t) b);
    }
    if (b->codec_output_rb) {
        arb_deinit(b->codec_output_rb);
    }
    if (b->http_output_rb) {
        arb_deinit(b->http_output_rb);
    }
    if (b->http_stream) {
        audio_stream_destroy(&b->http_stream->base);
        http_playback_stream_destroy(b->http_stream);
    }
    if (b->lock) {
        vSemaphoreDelete(b->lock);
    }

    free(b);
}
