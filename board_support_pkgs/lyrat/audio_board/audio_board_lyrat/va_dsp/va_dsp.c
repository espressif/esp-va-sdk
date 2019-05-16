// Copyright 2018 Espressif Systems (Shanghai) PTE LTD
// All rights reserved.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>

#include <esp_log.h>
#include <esp_wwe.h>
#include <i2s_stream.h>
#include <media_hal.h>
#include <audio_board.h>
#include <ringbuf.h>
#include <voice_assistant.h>
#include <speech_recognizer.h>
#include <va_mem_utils.h>
#include <media_hal.h>
#include "resampling.h"
#include "va_dsp.h"

#define WWE_TASK_STACK (8 * 1024)
#define DETECT_SAMP_RATE 16000UL
#define SAMP_RATE 48000UL
#define SAMP_BITS 16
#define SAMPLE_FRAME 320
#define SAMPLE_MS 20
#define PCM_SIZE (4 * 1024)
//Sample size for 20millisec data on 48KHz/16bit sampling. Division factor is (sectomillisec * bitsinbytes)
#define SAMPLE_SZ ((SAMP_RATE * I2S_BITS_PER_SAMPLE_16BIT * SAMPLE_MS) / (1000 * 8))
static const char *TAG = "dsp";
bool btn_mute_press;

static struct dsp_data {
    int item_chunk_size;
    bool detect_wakeword;
    bool speech_recog_en;
    QueueHandle_t recog_queue;
    ringbuf_t *temp_rb;
    audio_resample_config_t resample;
    i2s_stream_t *read_i2s_stream;
    TaskHandle_t nn_task_handle;
    int pcm_stored_data;
    bool write_to_store;
    int16_t data_buf[SAMPLE_SZ];
    char pcm_store[PCM_SIZE];
    bool dsp_inited;
} dd;

static media_hal_config_t media_hal_conf = {
    .op_mode    = MEDIA_HAL_MODE_SLAVE,
    .adc_input  = MEDIA_HAL_ADC_INPUT_LINE1,
    .dac_output = MEDIA_HAL_DAC_OUTPUT_ALL,
    .codec_mode = MEDIA_HAL_CODEC_MODE_BOTH,
    .bit_length = MEDIA_HAL_BIT_LENGTH_16BITS,
    .format     = MEDIA_HAL_I2S_NORMAL,
    .port_num = 0,
};

static esp_err_t reader_stream_event_handler(void *arg, int event, void *data)
{
    ESP_LOGI(TAG, "Reader stream event %d", event);
    return ESP_OK;
}

static ssize_t dsp_write_cb(void *h, void *data, int len, uint32_t wait)
{
    ssize_t sent_len;
    if(len == 0) {
        return 0;
    }
    sent_len = rb_write(dd.temp_rb, data, len, wait);
    return sent_len;
}

int va_app_speech_stop()
{
    ESP_LOGI(TAG, "Sending stop command");
    dd.speech_recog_en = false;
    vTaskDelay(20/portTICK_RATE_MS);
    dd.detect_wakeword = true;
    ESP_LOGI(TAG, "Stopped I2S audio stream");
    return ESP_OK;
}

int va_app_speech_start()
{
    ESP_LOGI(TAG, "Sending start command");
    dd.detect_wakeword = false;
    ESP_LOGI(TAG, "Starting I2S audio stream");
    dd.speech_recog_en = true;
    return ESP_OK;
}

void va_dsp_reset()
{
    return;
}

void va_dsp_mic_mute(bool va_dsp_mic_mt)
{
    btn_mute_press = va_dsp_mic_mt;
}

int va_dsp_tap_to_talk_start()
{
    if (!dd.dsp_inited)
        return -1;

    ESP_LOGI(TAG, "Sending start command");
    dd.detect_wakeword = false;
    ESP_LOGI(TAG, "Starting I2S audio stream");
    dd.pcm_stored_data = 0;
    dd.write_to_store = true;
    return 0;
}

static void read_rb_task(void *arg)
{
    size_t sent_len;
    while(1) {
        if(btn_mute_press) {
            //do nothing!!
            vTaskDelay(200/portTICK_RATE_MS);
        } else {
            rb_read(dd.temp_rb, (uint8_t *)dd.data_buf, SAMPLE_SZ * 2, portMAX_DELAY);
            sent_len = audio_resample((short *)dd.data_buf, (short *)dd.data_buf, SAMP_RATE, DETECT_SAMP_RATE, SAMPLE_SZ, SAMPLE_SZ, 2, &dd.resample);
            sent_len = audio_resample_down_channel((short *)dd.data_buf, (short *)dd.data_buf, DETECT_SAMP_RATE, DETECT_SAMP_RATE, sent_len, SAMPLE_SZ, 0, &dd.resample);
            sent_len = sent_len * 2;  //convert 16bit lengtth to number of bytes
            if(dd.detect_wakeword) {
                xQueueSend(dd.recog_queue, dd.data_buf, 0);
            } else if(dd.speech_recog_en) {
                speech_recognizer_record(dd.data_buf, sent_len);
                //printf("recorded speech %d\n", sent_len);
            } else if (dd.write_to_store) {
                if ( (dd.pcm_stored_data + sent_len) < sizeof(dd.pcm_store)) {
                    //printf("Writing to store at pcm_stored_data %d %d sizeof %d\n", dd.pcm_stored_data, sent_len, sizeof(dd.pcm_store));
                    memcpy(dd.pcm_store + dd.pcm_stored_data, dd.data_buf, sent_len);
                    dd.pcm_stored_data += sent_len;
                } else {
                    /* store buffer is full, raise the 'Recognize' event, and flush the data */
                    ESP_LOGI(TAG, "Sending recognize command");
                    speech_recognizer_recognize(0, TAP);
                    speech_recognizer_record(dd.pcm_store, dd.pcm_stored_data);
                    ESP_LOGI(TAG, "Flushed store data: %d\n", dd.pcm_stored_data);
                    //Send data which is not flushed in buffer
                    speech_recognizer_record(dd.data_buf, sent_len);
                    dd.write_to_store = false;
                    dd.speech_recog_en = true;
                }
            }
        }
    }
}

static void nn_task(void *arg)
{
    int frequency = esp_wwe_get_sample_rate();
    int audio_chunksize = esp_wwe_get_sample_chunksize();

    int16_t *buffer = malloc(audio_chunksize*sizeof(int16_t));
    assert(buffer);
    int chunks=0;
    int priv_ms = 0;
    while(1) {
        if (dd.detect_wakeword) {
            xQueueReceive(dd.recog_queue, buffer, portMAX_DELAY);
            int r = esp_wwe_detect(buffer);
            if (r && dd.detect_wakeword) {
                int new_ms = (chunks*audio_chunksize*1000)/frequency;
                printf("%.2f: Neural network detection triggered output %d.\n", (float)new_ms/1000.0, r);
                int x = (new_ms - priv_ms);
                priv_ms = new_ms;
                if(x != 20) {
                    va_dsp_tap_to_talk_start();
                }
            }
            chunks++;
        } else {
            memset(buffer, 0, (audio_chunksize * 2));
            vTaskDelay(100/portTICK_RATE_MS);
        }
    }
}

void va_dsp_init(void)
{
    dd.temp_rb = rb_init("nn-recog", 4 * 1024);
    i2s_stream_config_t i2s_cfg;
    memset(&i2s_cfg, 0, sizeof(i2s_cfg));
    i2s_cfg.i2s_num = 0;
    audio_board_i2s_init_default(&i2s_cfg.i2s_config);
    i2s_cfg.media_hal_cfg = media_hal_init(&media_hal_conf);

    dd.read_i2s_stream = i2s_reader_stream_create(&i2s_cfg);
    if (dd.read_i2s_stream) {
        ESP_LOGI(TAG, "Created I2S audio stream");
    } else {
        ESP_LOGE(TAG, "Failed creating I2S audio stream");
    }
    i2s_stream_set_stack_size(dd.read_i2s_stream, 5000);

    audio_io_fn_arg_t stream_reader_fn = {
        .func = dsp_write_cb,
        .arg = NULL,
    };
    audio_event_fn_arg_t stream_event_fn = {
        .func = reader_stream_event_handler,
    };
    if (audio_stream_init(&dd.read_i2s_stream->base, "i2s_reader", &stream_reader_fn, &stream_event_fn) != 0) {
        ESP_LOGE(TAG, "Failed creating audio stream");
        i2s_stream_destroy(dd.read_i2s_stream);
        dd.read_i2s_stream = NULL;
    }
    //Initialize NN model
    if (esp_wwe_init() != ESP_OK) {
        ESP_LOGE(TAG, "Failed to init ESP-WWE");
        return;
    }

    //Initialize sound source
    dd.item_chunk_size = esp_wwe_get_sample_chunksize() * sizeof(int16_t);
    dd.recog_queue = xQueueCreate(1, dd.item_chunk_size);
    xTaskCreate(&nn_task, "nn", WWE_TASK_STACK, NULL, (CONFIG_ESP32_PTHREAD_TASK_PRIO_DEFAULT - 1), &dd.nn_task_handle);
    xTaskCreate(&read_rb_task, "rb read task", WWE_TASK_STACK, NULL, (CONFIG_ESP32_PTHREAD_TASK_PRIO_DEFAULT - 1), NULL);

    audio_stream_start(&dd.read_i2s_stream->base);
    vTaskDelay(10/portTICK_RATE_MS);
    audio_stream_stop(&dd.read_i2s_stream->base);
    i2s_set_clk(I2S_NUM_0, SAMP_RATE, SAMP_BITS, I2S_CHANNEL_STEREO);
    vTaskDelay(10/portTICK_RATE_MS);
    audio_stream_start(&dd.read_i2s_stream->base);
    dd.detect_wakeword = true;
    dd.dsp_inited = true;
    media_hal_powerup(media_hal_get_handle());
    va_boot_dsp_signal();
}

