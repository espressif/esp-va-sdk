// Copyright 2018 Espressif Systems (Shanghai) PTE LTD
// All rights reserved.

#include <string.h>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <va_mem_utils.h>
#include <str_utils.h>
#include <audio_player.h>

#include "local_player.h"

#include "esp_vfs_fat.h"
#include "driver/sdmmc_host.h"
#include "driver/sdmmc_defs.h"
#include "sdmmc_cmd.h"
#define SD_CARD_OPEN_FILE_NUM_MAX 5

static const char *TAG = "[local_player]";

enum local_player_states {
    LOCAL_PLAYER_IDLE,
    LOCAL_PLAYER_PLAYING,
    LOCAL_PLAYER_PAUSED,
    LOCAL_PLAYER_STOPPED,
    LOCAL_PLAYER_FINISHED,
};

struct local_player {
    enum local_player_states current_state;
} local_player;

static void local_player_notify(const char *token, uint32_t offset, enum event_type type)
{
    ESP_LOGI(TAG, "Notify event type: %d", type);
    switch (type) {
        case AP_NOTIFY_CLEAR_QUEUE:
            break;

        case AP_NOTIFY_ERROR:
            break;

        case AP_NOTIFY_PB_STARTED:
            local_player.current_state = LOCAL_PLAYER_PLAYING;
            break;

        case AP_NOTIFY_PB_STOPPED:
            local_player.current_state = LOCAL_PLAYER_STOPPED;
            break;

        case AP_NOTIFY_PB_PAUSED:
            local_player.current_state = LOCAL_PLAYER_PAUSED;
            break;

        case AP_NOTIFY_PB_RESUMED:
            local_player.current_state = LOCAL_PLAYER_PLAYING;
            break;

        case AP_NOTIFY_PB_FINISHED:
            local_player.current_state = LOCAL_PLAYER_FINISHED;
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

int local_player_play(char *url, enum play_behaviour play_behaviour)
{
    printf("Playing: %s, %d\n",url, play_behaviour);
    char *token = va_mem_strdup("local_token", VA_MEM_EXTERNAL);
    audio_player_play(url, token, 0, play_behaviour, 0, 0, local_player_notify);
    return 0;
}

int local_player_stop()
{
    audio_player_stop(local_player_notify);
    return 0;
}



void local_player_task()
{
    char *url = NULL;
    while(1) {
        vTaskDelay(10000 / portTICK_PERIOD_MS);
        url = va_mem_strdup("file:///sdcard/1.mp3", VA_MEM_EXTERNAL);
        local_player_play(url, REPLACE_ALL);    
        vTaskDelay(10000 / portTICK_PERIOD_MS);
        url = va_mem_strdup("file:///sdcard/2.mp3", VA_MEM_EXTERNAL);
        local_player_play(url, REPLACE_ALL);
        vTaskDelay(10000 / portTICK_PERIOD_MS);
        url = va_mem_strdup("file:///sdcard/3.mp3", VA_MEM_EXTERNAL);
        local_player_play(url, ENQUEUE);
        vTaskDelay(10000 / portTICK_PERIOD_MS);
        url = va_mem_strdup("file:///sdcard/1.mp3", VA_MEM_EXTERNAL);
        local_player_play(url, ENQUEUE);
        while (1)
            vTaskDelay(portMAX_DELAY);
    }
}

static int sd_mmc_config()
{
    sdmmc_host_t host = SDMMC_HOST_DEFAULT();
    // To use 1-line SD mode, uncomment the following line:
    host.flags = SDMMC_HOST_FLAG_1BIT;
    sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();

    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = SD_CARD_OPEN_FILE_NUM_MAX
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

int local_player_init()
{
    TaskHandle_t handle;
    local_player.current_state = LOCAL_PLAYER_IDLE;
    ESP_ERROR_CHECK(sd_mmc_config());
    xTaskCreate(local_player_task, "local_player", 8192, NULL, 5, &handle);
    return 0;
}