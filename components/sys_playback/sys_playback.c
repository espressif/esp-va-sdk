// Copyright 2018 Espressif Systems (Shanghai) PTE LTD
// All rights reserved.

#include <string.h>
#include <esp_log.h>
#include <basic_rb.h>
#include <esp_err.h>
#include <resampling.h>
#include <audio_board.h>
#include <hollow_stream.h>
#include <va_dsp.h>
#include "sys_playback.h"
#include "media_hal_playback.h"
#include <esp_audio_mem.h>
#include <esp_downmix.h>

#define PB_DEFAULT_STACK_SIZE   (3 * 1024)
#define PB_DOWNMIX_STACK_SIZE   (4 * 1024)
#define PB_TASK_PRIORITY        5
#define PB_DOWNMIX_PRIORITY     5
#define PB_DEFAULT_BUF_SIZE     512
#define PB_BUFFER_SIZE          (12 * 512) /* 12x can handle 8k/1 --> 48k/2 */
#define OUT_SAMPLING_RATE       48000

static const char *TAG = "[sys_playback]";

static struct {
    hollow_stream_t *hollow_stream;
    /* Where the i2s stream comes to rest */
    rb_handle_t resting_rb;
    bool downmix_support;   //If downmix support is enabled
    /**
     * This is where we write downmixed data.
     * sys_playback_consume_buffer reads from this buffer and calls va_playback_data.
     */
    rb_handle_t downmix_rb;
    void *downmix_handle;
    /* If present, the tone gets priority */
    sys_playback_requester_t *tone;
    /* The currently playing playback requester */
    sys_playback_requester_t *current;
    sys_playback_requester_t *duck;
    SemaphoreHandle_t duck_lock;
    sys_playback_requester_t dummy;
    bool acquired;
    bool playback_starting_sent;
} sp;

static ssize_t sys_playback_dummy_read_cb(void *cb_data, void *data, int len, unsigned int wait)
{
    rb_handle_t rb = (rb_handle_t)cb_data;
    return rb_read(rb, (uint8_t *)data, len, wait);
}

static void sys_playback_dummy_wakeup_read_cb(void *cb_data)
{
    rb_handle_t rb = (rb_handle_t)cb_data;
    rb_wakeup_reader(rb);
}

uint32_t sys_playback_get_current_offset(sys_playback_requester_t *requester)
{
    int sampling_freq_in_khz = requester->audio_info.sample_rate / 1000;
    /* Frequency x Channels x 2-bytes (per sample) */
    int divisor = sampling_freq_in_khz * requester->audio_info.channels * 2;
    if (divisor) {
        return requester->samples_cnt / divisor;
    } else {
        return 0;
    }
}

int sys_playback_play_data(media_hal_audio_info_t *audio_info, void *buf, ssize_t len)
{
    ssize_t sent_len = 0;
    if (sp.playback_starting_sent == false) {
        va_dsp_playback_starting();
        sp.playback_starting_sent = true;
    } else {
        va_dsp_playback_ongoing();
    }
    sent_len = media_hal_playback(audio_info, (void *) buf, len);
    return sent_len;
}

/**
 * The function keeps reading data from main audio and ducked audio,
 * resamples+mixes it and writes to downmix_rb.
 */
static void sys_playback_task()
{
#define DATA_BUF_SIZE   (512)
    int prev_remain = 0;
    char *data = (char *) esp_audio_mem_calloc(1, DATA_BUF_SIZE);
    unsigned char *duck_buffer  = NULL;
    unsigned char *conv_main_buf = NULL;
    unsigned char *conv_duck_buf = NULL;
    int wait = portMAX_DELAY;

    if (sp.downmix_support) {
        duck_buffer  = (unsigned char *) esp_audio_mem_calloc(1, PB_BUFFER_SIZE);
        conv_main_buf = (unsigned char *) esp_audio_mem_calloc(1, PB_BUFFER_SIZE);
        conv_duck_buf = (unsigned char *) esp_audio_mem_calloc(1, PB_BUFFER_SIZE);
    }

    audio_resample_config_t resample_main = {0};
    audio_resample_config_t resample_duck = {0};
    downmix_status_t downmix_status = DOWNMIX_SWITCH_ON;

    while (1) {
        int conv_main_len = 0, conv_duck_len = 0;
        sys_playback_requester_t *active = sp.current;
        int data_read = 0, duck_read = 0;
        unsigned int wait_main = wait, wait_duck = 2;

        if (sp.tone) {
            /* Tone gets priority */
            active = sp.tone;
        }

        if (active == &sp.dummy) {
            if (!sp.duck) {
                /* Nothing to play! Raise va_dsp_playback_stopped event */
                (void) va_dsp_playback_stopped();
                sp.playback_starting_sent = false;
            } else if (sp.downmix_support) {
                /* active == &dummy, sp.duck is there and downmix feature is enabled */
                wait_duck = wait;
                wait_main = 0;
            }
        }

        /**** Read and Process Main Data ****/
        data_read = active->read_cb(active->cb_data, data, DATA_BUF_SIZE, wait_main);

        if (data_read == RB_READER_UNBLOCK) {
            /* Just a wakeup, do nothing and go for duck audio. */
        } else if (data_read > 0) {
            active->samples_cnt += data_read;
            if (sp.downmix_support) {
                /* Resample to OUT_SAMPLING_RATE */
                conv_main_len = audio_resample((short *) data, (short *) conv_main_buf, active->audio_info.sample_rate, OUT_SAMPLING_RATE,
                                               data_read / 2, PB_BUFFER_SIZE, active->audio_info.channels, &resample_main);

                if (active->audio_info.channels == 1) {
                    conv_main_len = audio_resample_up_channel((short *) conv_main_buf, (short *) conv_main_buf, OUT_SAMPLING_RATE, OUT_SAMPLING_RATE,
                                                              conv_main_len, PB_BUFFER_SIZE, &resample_main);
                }
            } else {
                sys_playback_play_data(&active->audio_info, data, data_read);
            }
        } else if (data_read < 0) {
            /* If this was a tone, it has been completely played out, reset the pointer now */
            if (active == sp.tone) {
                sp.tone = NULL;
            }
            prev_remain = 0;
        }
        /**** Main Data Done ****/

        if (sp.downmix_support) {
            /**** Read and Process Ducked Data ****/
            xSemaphoreTake(sp.duck_lock, portMAX_DELAY);
            if (sp.duck) {
                int duck_to_read = DATA_BUF_SIZE;
                if (conv_main_len && conv_main_len < prev_remain) {
                    duck_to_read = 0;
                    conv_duck_len = prev_remain;
                } else if (conv_main_len) {
                    /* How many bytes to read to get out bytes = conv_main_len * 2? */
                    duck_to_read = ((conv_main_len - prev_remain) * sp.duck->audio_info.sample_rate) / OUT_SAMPLING_RATE;
                    duck_to_read = (duck_to_read + 3) & ~(int) 0x01; /* Read few extra and make it multiple of 2 */
                    duck_to_read *= sp.duck->audio_info.channels;
                }

                duck_read = sp.duck->read_cb(sp.duck->cb_data, duck_buffer, duck_to_read, wait_duck);

                if (duck_read == RB_READER_UNBLOCK && data_read <= 0) {
                    /* Just a wakeup, simply return */
                    xSemaphoreGive(sp.duck_lock);
                    continue;
                } else if (duck_read < 0) {
                    conv_duck_len = 0;
                } else if (duck_read > 0) {
                    sp.duck->samples_cnt += duck_read;
                    /* Resample to OUT_SAMPLING_RATE. */
                    conv_duck_len = audio_resample((short *) duck_buffer, (short *) conv_duck_buf + prev_remain, sp.duck->audio_info.sample_rate, OUT_SAMPLING_RATE,
                                               duck_read / 2, PB_BUFFER_SIZE - 2 * prev_remain, sp.duck->audio_info.channels, &resample_duck);

                    if (sp.duck->audio_info.channels == 1) {
                        conv_duck_len = audio_resample_up_channel((short *) conv_duck_buf + prev_remain, (short *) conv_duck_buf + prev_remain, OUT_SAMPLING_RATE, OUT_SAMPLING_RATE,
                                                              conv_duck_len, PB_BUFFER_SIZE - 2 * prev_remain, &resample_duck);
                    }
                }
                conv_duck_len += prev_remain;
                prev_remain = 0;
            }
            /**** Duck Data Done ****/

            xSemaphoreGive(sp.duck_lock);

            /**** Downmix and write data to downmix_rb ****/
            if (conv_main_len && conv_duck_len) { /* Mix main and ducked audio */
                prev_remain = 0;
                if (conv_main_len > conv_duck_len) {
                    /**
                     * Happens only when we are not able to read enough duck audio.
                     * This should be seldom thing.
                     */
                    bzero(conv_duck_buf + 2 * conv_duck_len, 2 * (conv_main_len - conv_duck_len));
                }
                /* Otherwise we are anyway handling this properly.! */
                esp_downmix_process(sp.downmix_handle, conv_main_buf, conv_main_len * 2, conv_duck_buf, conv_main_len * 2, duck_buffer, downmix_status);
                rb_write(sp.downmix_rb, duck_buffer, conv_main_len * 2, wait);
                if (conv_duck_len > conv_main_len) {
                    prev_remain = conv_duck_len - conv_main_len;
                    memmove(conv_duck_buf, conv_duck_buf + conv_main_len * 2, prev_remain * 2);
                }
            } else if (conv_main_len) { /* Just main audio */
                rb_write(sp.downmix_rb, conv_main_buf, conv_main_len * 2, wait);
            } else if (conv_duck_len) { /* Only ducked audio! Downmix this with silence. */
                bzero(conv_main_buf, conv_duck_len * 2); /* Silence. */
                esp_downmix_process(sp.downmix_handle, conv_main_buf, conv_duck_len * 2, conv_duck_buf, conv_duck_len * 2, duck_buffer, downmix_status);
                rb_write(sp.downmix_rb, duck_buffer, conv_duck_len * 2, wait);
            }
        }
    }

    /**
     * We never exit the while loop and the task, but let's keep it clean.
     */
    if (duck_buffer) {
        esp_audio_mem_free(duck_buffer);
    }
    if (conv_main_buf) {
        esp_audio_mem_free(conv_main_buf);
    }
    if (conv_duck_buf) {
        esp_audio_mem_free(conv_duck_buf);
    }
    vTaskDelete(NULL);
#undef DATA_BUF_SIZE
}

/**
 * This function reads data from downmix_rb,
 * and calls va_app_playback_data to write it to i2s.
 * Ideally, this function should never exit.
 */
static void sys_playback_downmix_consumer_task(void *arg)
{
    int read_size = 512;
    char data[read_size];
    /**
     * Read data from downmixed buffer and call va_app_playback_data
     */
    media_hal_audio_info_t audio_info = {
        .sample_rate = OUT_SAMPLING_RATE,
        .channels = 2,
        .bits_per_sample = 16,
    };

    while (1) {
        int bytes_read = rb_read(sp.downmix_rb, (void *) data, read_size, portMAX_DELAY);
        if (bytes_read > 0) {
            sys_playback_play_data(&audio_info, data, bytes_read);
        }
    }
}

/* This is fairly similar to acquire */
int sys_playback_play_tone(sys_playback_requester_t *tone)
{
    sp.tone = tone;
    if (sp.current->wakeup_reader_cb) {
        sp.current->wakeup_reader_cb(sp.current->cb_data);
    }
    return 0;
}

bool sys_playback_downmix_supported()
{
    return sp.downmix_support;
}
/**
 * Put requester in focus(as main audio).
 * We should check here if duck audio is same as this, in which case we simply remove duck one and add to main/current audio.
 * Note that we should already have removed main audio before doing this.
 */
int sys_playback_acquire(sys_playback_requester_t *requester)
{
    ESP_LOGI(TAG, "Acquire");
    if (sp.acquired) {
        ESP_LOGE(TAG, "Acquiring playback that wasn't released");
        return -1;
    }
    if (requester) {
        sp.current = requester;
    } else {
        /* Nothing's playing */
        sp.current = &sp.dummy;
    }
    /* Since the playback wasn't acquired, the callback would be blocked on the resting ring-buffer */
    sp.dummy.wakeup_reader_cb(sp.dummy.cb_data);
    sp.acquired = true;
    return 0;
}

/**
 * Register a duck audio. If ducked playback exists, it will simply be replaced with newer one.
 */
int sys_playback_put_ducked(sys_playback_requester_t *requester)
{
    ESP_LOGI(TAG, "Duck");
    xSemaphoreTake(sp.duck_lock, portMAX_DELAY);
    sp.duck = requester;
    xSemaphoreGive(sp.duck_lock);
    return 0;
}

/**
 * Remove ducked audio.
 *
 * `requester` must match current `duck`, else will do nothing.
 */
int sys_playback_remove_ducked(sys_playback_requester_t *requester)
{
    xSemaphoreTake(sp.duck_lock, portMAX_DELAY);
    if (sp.duck == requester) {
        /* Remove duck only if reuester is same as ducked requester */
        sp.duck = NULL;
    }
    xSemaphoreGive(sp.duck_lock);
    return 0;
}

/**
 * Release main audio.
 * We could still be having duck audio! When will we move it to focus?
 */
int sys_playback_release(void)
{
    ESP_LOGI(TAG, "Release");
    if (!sp.acquired) {
        ESP_LOGE(TAG, "Releasing playback that wasn't acquired");
        return -1;
    }

    wakeup_reader_cb_t old_wakeup_reader_cb = NULL;
    void *old_cb_data = NULL;
    if (sp.current->wakeup_reader_cb) {
        old_wakeup_reader_cb = sp.current->wakeup_reader_cb;
        old_cb_data = sp.current->cb_data;
    }
    sp.current = &sp.dummy;
    if (old_wakeup_reader_cb) {
        /* Wakeup the reader, in case it was blocked on the old rb */
        old_wakeup_reader_cb(old_cb_data);
    }
    sp.acquired = false;
    return 0;
}

static void sys_playback_downmix_deinit() {
    if (sp.downmix_rb) {
        rb_cleanup(sp.downmix_rb);
        sp.downmix_rb = NULL;
    }
    if (sp.downmix_handle) {
        esp_downmix_close(sp.downmix_handle);
        sp.downmix_handle = NULL;
    }
}

static esp_err_t sys_playback_downmix_init(sys_playback_config_t *sys_playback_cfg)
{
    (void) sys_playback_cfg; /* Unused */

    //init downmix_info structure.
    esp_downmix_info_t downmix_info;
    bzero(&downmix_info, sizeof (esp_downmix_info_t));
    downmix_info.sample_rate = OUT_SAMPLING_RATE;
    downmix_info.bits_num = 16;
    downmix_info.dual_2_mono_select_ch = 2;
    downmix_info.downmix_gain[0].set_dbgain[1] = 0;
    downmix_info.downmix_gain[1].set_dbgain[1] = -20;
    downmix_info.channels[0] = 2;
    downmix_info.channels[1] = 2;
    sp.downmix_handle = esp_downmix_open(&downmix_info);
    if (!sp.downmix_handle) {
        ESP_LOGE(TAG, "Could not open downmix!");
        return ESP_FAIL;
    }
    sp.downmix_rb = rb_init("downmix_rb", PB_BUFFER_SIZE);
    if (sp.downmix_rb == NULL) {
        ESP_LOGE(TAG, "failed to create downmix_rb");
        sys_playback_downmix_deinit();
        return ESP_FAIL;
    }
#if 0
    /* Assign default params */
    hollow_stream_cfg.hollow_stream_stack_sz = PB_DEFAULT_STACK_SIZE;
    hollow_stream_cfg.hollow_stream_task_priority = PB_DEFAULT_TASK_PRIORITY;
    hollow_stream_cfg.hollow_stream_buf_size = PB_DEFAULT_BUF_SIZE;

    if (sys_playback_cfg) {
        /* Overwrite with provided one */
        if (sys_playback_cfg->stack_size) {
            hollow_stream_cfg.hollow_stream_stack_sz = sys_playback_cfg->stack_size;
        }
        if (!sys_playback_cfg->task_priority) {
            hollow_stream_cfg.hollow_stream_task_priority = sys_playback_cfg->task_priority;
        }
        if (!sys_playback_cfg->buf_size) {
            hollow_stream_cfg.hollow_stream_buf_size = sys_playback_cfg->buf_size;
        }
    }
#endif
    return ESP_OK;;
}

int sys_playback_init(sys_playback_config_t *sys_playback_cfg)
{
    static bool sp_initialized = false;
    if (sp_initialized) {
        ESP_LOGI(TAG, "Already initialized");
        return ESP_OK;
    }

    sp.resting_rb = rb_init("resting_rb", 512);
    if (sp.resting_rb == NULL) {
        ESP_LOGE(TAG, "failed to create resting rb");
        return ESP_FAIL;
    }
    memset(&sp.dummy, 0, sizeof(sp.dummy));
    /* Use the dummy rb */
    sp.dummy.read_cb = sys_playback_dummy_read_cb;
    sp.dummy.wakeup_reader_cb = sys_playback_dummy_wakeup_read_cb;
    sp.dummy.cb_data = sp.resting_rb;
    sp.current = &sp.dummy;

    if (sys_playback_cfg) {
        sp.downmix_support = sys_playback_cfg->downmix_support;
    }

    sp.duck = NULL;
    sp.duck_lock = xSemaphoreCreateMutex();

    if (sp.downmix_support) {
        /* Initialize and create downmix handle */
        if (sys_playback_downmix_init(sys_playback_cfg) == ESP_FAIL) {
            ESP_LOGW(TAG, "Downmix initialization failed. Downmixing will be disabled...");
            sp.downmix_support = false;
        }
    }

    if (sp.downmix_support) {
        if (xTaskCreate(sys_playback_downmix_consumer_task, "sys_pb_downmix_writer", PB_DEFAULT_STACK_SIZE, NULL, PB_TASK_PRIORITY, NULL) != pdPASS) {
            ESP_LOGE(TAG, "Error creating sys_playback_downmix_consumer_task task! Downmixing will be disabled...");
            /* Downmix cleanups */
            sys_playback_downmix_deinit();
            sp.downmix_support = false;
        }
    }

    if (xTaskCreate(sys_playback_task, "sys_pb_task", PB_DOWNMIX_STACK_SIZE, NULL, PB_DOWNMIX_PRIORITY, NULL) != pdPASS) {
        ESP_LOGE(TAG, "Error creating sys_playback_task");
        goto sp_init_error;
    }

    sp_initialized = true;
    return ESP_OK;

sp_init_error:
    sys_playback_downmix_deinit();
    rb_cleanup(sp.resting_rb);
    return ESP_FAIL;
}
