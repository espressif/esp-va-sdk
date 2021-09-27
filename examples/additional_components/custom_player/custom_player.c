// Copyright 2018 Espressif Systems (Shanghai) PTE LTD
// All rights reserved.

#define USE_HTTP 1
#define USE_SPIFFS 0
#define USE_SDCARD 0

#include <string.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_log.h>
#include <va_mem_utils.h>
#include <str_utils.h>
#include <audio_player.h>

#include "custom_player.h"

#if USE_SDCARD
#include "esp_vfs_fat.h"
#include "driver/sdmmc_host.h"
#include "driver/sdmmc_defs.h"
#include "sdmmc_cmd.h"

#define URL_PREFIX "file:///sdcard"
#endif /* USE_SDCARD */

#if USE_SPIFFS
#include <esp_spiffs.h>

#define URL_PREFIX "file:///spiffs"
#endif /* USE_SPIFFS */

#if USE_HTTP
#define URL_PREFIX "https://raw.githubusercontent.com/wiki/espressif/esp-va-sdk/custom_audio"
#endif /* USE_HTTP */

#define MAX_OPEN_FILES 5

static const char *TAG = "[custom_player]";

enum custom_player_states {
    CUSTOM_PLAYER_IDLE,
    CUSTOM_PLAYER_PLAYING,
    CUSTOM_PLAYER_PAUSED,
    CUSTOM_PLAYER_STOPPED,
    CUSTOM_PLAYER_FINISHED,
};

struct custom_player {
    enum custom_player_states current_state;
} custom_player;

static void custom_player_notify(const char *token, uint32_t offset, enum event_type type)
{
    ESP_LOGI(TAG, "Notify event type: %d", type);
    switch (type) {
        case AP_NOTIFY_CLEAR_QUEUE:
            break;

        case AP_NOTIFY_ERROR:
            break;

        case AP_NOTIFY_PB_STARTED:
            custom_player.current_state = CUSTOM_PLAYER_PLAYING;
            break;

        case AP_NOTIFY_PB_STOPPED:
            custom_player.current_state = CUSTOM_PLAYER_STOPPED;
            break;

        case AP_NOTIFY_PB_PAUSED:
            custom_player.current_state = CUSTOM_PLAYER_PAUSED;
            break;

        case AP_NOTIFY_PB_RESUMED:
            custom_player.current_state = CUSTOM_PLAYER_PLAYING;
            break;

        case AP_NOTIFY_PB_FINISHED:
            custom_player.current_state = CUSTOM_PLAYER_FINISHED;
            break;

        case AP_NOTIFY_PB_DOWNLOAD_FINISHED:
            break;

        case AP_NOTIFY_DELAY_ELAPSED:
            break;

        case AP_NOTIFY_INTERVAL_ELAPSED:
            break;

        default:
            break;
    }
}

int custom_player_play(char *url, enum play_behaviour play_behaviour)
{
    printf("Playing: %s, %d\n",url, play_behaviour);
    char *token = va_mem_strdup("custom_token", VA_MEM_EXTERNAL);
    audio_player_play(url, token, 0, play_behaviour, 0, 0, custom_player_notify);
    return 0;
}

int custom_player_stop()
{
    audio_player_stop(custom_player_notify);
    return 0;
}

void custom_player_task()
{
    char *url = NULL;
    while(1) {
        vTaskDelay(10000 / portTICK_PERIOD_MS);

        url = va_mem_strdup(URL_PREFIX"/1.mp3", VA_MEM_EXTERNAL);
        /* 1.mp3 will replace anything that is currently playing. */
        custom_player_play(url, REPLACE_ALL);
        vTaskDelay(10000 / portTICK_PERIOD_MS);

        url = va_mem_strdup(URL_PREFIX"/2.mp3", VA_MEM_EXTERNAL);
        /* 2.mp3 will replace anything that is currently playing. */
        custom_player_play(url, REPLACE_ALL);
        vTaskDelay(10000 / portTICK_PERIOD_MS);

        url = va_mem_strdup(URL_PREFIX"/3.mp3", VA_MEM_EXTERNAL);
        /* 3.mp3 will wait for the currently playing to get over. */
        custom_player_play(url, ENQUEUE);
        vTaskDelay(10000 / portTICK_PERIOD_MS);

        url = va_mem_strdup(URL_PREFIX"/1.mp3", VA_MEM_EXTERNAL);
        /* 1.mp3 will wait for the currently playing to get over. */
        custom_player_play(url, ENQUEUE);

        while (1)
            vTaskDelay(portMAX_DELAY);
    }
}

#if USE_SDCARD
static int custom_player_sdcard_init()
{
    ESP_LOGI(TAG, "Initializing sdcard");
    sdmmc_host_t host = SDMMC_HOST_DEFAULT();
    // To use 1-line SD mode, uncomment the following line:
    host.flags = SDMMC_HOST_FLAG_1BIT;
    sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();

    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,        .max_files = MAX_OPEN_FILES
    };

    sdmmc_card_t *card;
    esp_err_t ret = esp_vfs_fat_sdmmc_mount("/sdcard", &host, &slot_config, &mount_config, &card);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error mounting SDMMC");
        return ret;
    }

    sdmmc_card_print_info(stdout, card);
    return ESP_OK;
}
#endif /* USE_SDCARD */

#if USE_SPIFFS
static int custom_player_spiffs_init()
{
    ESP_LOGI(TAG, "Initializing spiffs");
    esp_vfs_spiffs_conf_t conf = {
      .base_path = "/spiffs",
      .partition_label = NULL,
      .max_files = MAX_OPEN_FILES,
      .format_if_mount_failed = false
    };

    esp_err_t ret = esp_vfs_spiffs_register(&conf);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount or format filesystem");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            ESP_LOGE(TAG, "Failed to find SPIFFS partition");
        } else {
            ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
        }
        return ret;
    }

    size_t total = 0, used = 0;
    ret = esp_spiffs_info(conf.partition_label, &total, &used);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret));
    } else {
        ESP_LOGI(TAG, "Partition size: total: %d, used: %d", total, used);
    }
    return ret;
}
#endif /* USE_SPIFFS */

int custom_player_init()
{
    TaskHandle_t handle;
    custom_player.current_state = CUSTOM_PLAYER_IDLE;

#if USE_SDCARD
    custom_player_sdcard_init();
#endif /* USE_SDCARD */

#if USE_SPIFFS
    custom_player_spiffs_init();
#endif /* USE_SPIFFS */

    /* Initialise audio_player if not already done */
    audio_player_init();

    /* Create a task which tests custom_player */
    xTaskCreate(custom_player_task, "custom_player", 8192, NULL, 5, &handle);
    return 0;
}