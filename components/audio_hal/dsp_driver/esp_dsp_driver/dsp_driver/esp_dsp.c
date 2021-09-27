// Copyright 2018 Espressif Systems (Shanghai) PTE LTD
// All rights reserved.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>

#include <i2s_stream.h>
#include <audio_board.h>
#include <media_hal.h>
#include <esp_audio_mem.h>
#include <basic_rb.h>
#include <resampling.h>
#include <va_dsp.h>
#include <esp_dsp.h>
#include <common_dsp.h>

#define RB_TASK_STACK (8 * 1024)
#define RB_SIZE (4 * 1024)

#define DETECT_SAMP_RATE 16000UL
#define SAMP_BITS I2S_BITS_PER_SAMPLE_16BIT
#define SAMP_MS 20

static const char *TAG = "[esp_dsp]";

static struct dsp_data {
    rb_handle_t raw_mic_data;
    audio_resample_config_t resample;
    i2s_stream_t *read_i2s_stream;
    int16_t *data_buf;
    uint32_t data_sample_size;
    uint32_t sample_rate;
    int channels;
    int i2s_number; /* 0 or 1 */
    bool set_i2s_clk;
} dd = {
    .set_i2s_clk = true,
    .sample_rate = SAMPLE_RATE_DEFAULT,
    .channels = CHANNELS_DEFAULT,
    .i2s_number = I2S_NUMBER_DEFAULT
};

static esp_err_t reader_stream_event_handler(void *arg, int event, void *data)
{
    ESP_LOGI(TAG, "Reader stream event %d", event);
    return ESP_OK;
}

static ssize_t dsp_write_cb(void *h, void *data, int len, uint32_t wait)
{
    ssize_t sent_len;
    if(len <= 0) {
        return 0;
    }
    sent_len = rb_write(dd.raw_mic_data, data, len, wait);
    return sent_len;
}

static void resample_rb_data_task(void *arg)
{
    size_t sent_len;
    while(1) {
        sent_len = rb_read(dd.raw_mic_data, (uint8_t *)dd.data_buf, dd.data_sample_size * 2, portMAX_DELAY);
        sent_len = audio_resample((short *)dd.data_buf, (short *)dd.data_buf, dd.sample_rate, DETECT_SAMP_RATE,
                                    dd.data_sample_size, dd.data_sample_size, dd.channels, &dd.resample);
        if (dd.channels == 2) {
            sent_len = audio_resample_down_channel((short *)dd.data_buf, (short *)dd.data_buf, DETECT_SAMP_RATE,
                                                    DETECT_SAMP_RATE, sent_len, dd.data_sample_size, 0, &dd.resample);
        }
        sent_len = sent_len * 2;  //convert 16bit length to number of bytes
        common_dsp_write_mic_data(dd.data_buf, sent_len, 0);
    }
}

esp_err_t esp_dsp_audio_stream_pause()
{
    if(dd.read_i2s_stream && dd.read_i2s_stream->base.label) {
        audio_stream_pause(&dd.read_i2s_stream->base);
        vTaskDelay(100/portTICK_PERIOD_MS);
        return ESP_OK;
    }
    return ESP_FAIL;
}

esp_err_t esp_dsp_audio_stream_resume()
{
    if(dd.read_i2s_stream && dd.read_i2s_stream->base.label) {
        audio_stream_resume(&dd.read_i2s_stream->base);
        vTaskDelay(100/portTICK_PERIOD_MS);
        return ESP_OK;
    }
    return ESP_FAIL;
}

void esp_dsp_configure(esp_dsp_config_t *cfg)
{
    if (cfg) {
        dd.sample_rate = cfg->sample_rate;
        dd.channels = cfg->channels;
        dd.i2s_number = cfg->i2s_number;
        dd.set_i2s_clk = cfg->set_i2s_clk;
    }
}

void esp_dsp_init()
{
    //Sample size for 20millisec data on 48KHz/16bit sampling. Division factor is (sectomillisec * bitsinbytes)
    dd.data_sample_size = ((dd.sample_rate * SAMP_BITS * SAMP_MS) / (1000 * 8));
    dd.data_buf = esp_audio_mem_calloc(1, dd.data_sample_size * (sizeof (int16_t)));
    if (dd.data_buf == NULL) {
        ESP_LOGE(TAG, "dd.data_buf allocation failed!");
        return;
    }
    dd.raw_mic_data = rb_init("raw-mic", RB_SIZE);
    if (dd.raw_mic_data == NULL) {
        ESP_LOGE(TAG, "dd.raw_mic_data rb_init failed!");
        goto esp_dsp_init_error_exit;
    }

    i2s_stream_config_t i2s_cfg;
    memset(&i2s_cfg, 0, sizeof(i2s_cfg));
    i2s_cfg.i2s_num = dd.i2s_number;
    audio_board_i2s_init_default(&i2s_cfg.i2s_config);
    i2s_cfg.media_hal_cfg = media_hal_get_handle();

    dd.read_i2s_stream = i2s_reader_stream_create(&i2s_cfg);
    if (dd.read_i2s_stream) {
        ESP_LOGI(TAG, "Created I2S audio stream");
    } else {
        ESP_LOGE(TAG, "Failed creating I2S audio stream");
        goto esp_dsp_init_error_exit;
    }

    /* Override default stack size of 2.2K with 3K. 5K seems really unneccessary. */
    i2s_stream_set_stack_size(dd.read_i2s_stream, 3000);

    audio_io_fn_arg_t stream_reader_fn = {
        .func = dsp_write_cb,
        .arg = NULL,
    };
    audio_event_fn_arg_t stream_event_fn = {
        .func = reader_stream_event_handler,
    };
    if (audio_stream_init(&dd.read_i2s_stream->base, "i2s_reader", &stream_reader_fn, &stream_event_fn) != 0) {
        ESP_LOGE(TAG, "Failed creating audio stream");
        goto esp_dsp_init_error_exit;
    }

    audio_stream_start(&dd.read_i2s_stream->base);
    vTaskDelay(10/portTICK_RATE_MS);
    if (dd.set_i2s_clk) {
        audio_stream_stop(&dd.read_i2s_stream->base);
        i2s_set_clk(dd.i2s_number, dd.sample_rate, SAMP_BITS, dd.channels);
        vTaskDelay(10/portTICK_RATE_MS);
        audio_stream_start(&dd.read_i2s_stream->base);
    }

    xTaskCreate(&resample_rb_data_task, "rb read task", RB_TASK_STACK, NULL, (CONFIG_ESP32_PTHREAD_TASK_PRIO_DEFAULT - 1), NULL);
    return;

esp_dsp_init_error_exit:
    if (dd.data_buf) {
        free(dd.data_buf);
        dd.data_buf = NULL;
    }
    if (dd.raw_mic_data) {
        rb_cleanup(dd.raw_mic_data);
        dd.raw_mic_data = NULL;
    }
    if (dd.read_i2s_stream) {
        i2s_stream_destroy(dd.read_i2s_stream);
        dd.read_i2s_stream = NULL;
    }
}
