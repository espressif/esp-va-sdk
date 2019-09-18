/*
 * ESPRESSIF MIT License
 *
 * Copyright (c) 2018 <ESPRESSIF SYSTEMS (SHANGHAI) PTE LTD>
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
#include <esp_console.h>
#include <esp_heap_caps.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <va_mem_utils.h>
#include <va_nvs_utils.h>
#include <string.h>
#include <nvs.h>
#include <scli.h>
#include <diag_cli.h>
#include <voice_assistant.h>

static const char *TAG = "[va_diag_cli]";

static void hex_dump(uint8_t *data, int length)
{
    for (int i = 0; i < length; i++) {
        printf("0x%02x ", data[i]);
        if (i && ((i % 16) == 0)) {
            printf("\n");
        }
    }
    printf("\n");
}

static int nvs_get_cli_handler(int argc, char *argv[])
{
    const char *variable = argv[1];
    const char *type = argv[2];
    size_t val_length = 0;

    /* Just to go to the next line */
    printf("\n");
    if (argc != 3) {
        ESP_LOGE(TAG, "Incorrect arguments");
        return 0;
    }
    if (strcmp(type, "string") == 0) {
        va_nvs_get_str(variable, NULL, &val_length);
        if (val_length == 0) {
            printf("%s: Variable with 0 length\n", TAG);
            return 0;
        } else {
            char *value = va_mem_alloc(val_length, VA_MEM_EXTERNAL);
            if (value == NULL) {
                printf("%s: Memory allocation failed", TAG);
                return 0;
            }
            va_nvs_get_str(variable, value, &val_length);
            printf("\n%s\n", value);
            va_mem_free(value);
        }
    } else if (strcmp(type, "blob") == 0) {
        va_nvs_get_blob(variable, NULL, &val_length);
        if (val_length == 0) {
            printf("%s: Variable with 0 length\n", TAG);
            return 0;
        } else {
            uint8_t *value = va_mem_alloc(val_length, VA_MEM_EXTERNAL);
            if (value == NULL) {
                printf("%s: Memory allocation failed", TAG);
                return 0;
            }
            va_nvs_get_blob(variable, value, &val_length);
            hex_dump(value, val_length);
            va_mem_free(value);
        }
    } else if (strcmp(type, "i8") == 0) {
        int8_t value;
        va_nvs_get_i8(variable, &value);
        printf("\n%d\n", value);
        return 0;
    } else if (strcmp(type, "u16") == 0) {
        uint16_t value;
        va_nvs_get_u16(variable, &value);
        printf("\n%u\n", value);
        return 0;
    } else {
        ESP_LOGE(TAG, "Incorrect argument");
    }
    return 0;
}

static int nvs_set_cli_handler(int argc, char *argv[])
{
    const char *variable = argv[1];
    const char *type = argv[2];
    char *value = argv[3];

    /* Just to go to the next line */
    printf("\n");
    if (argc != 4) {
        ESP_LOGE(TAG, "Incorrect arguments");
        return 0;
    }
    if (strcmp(type, "string") == 0) {
        va_nvs_set_str(variable, value);
    } else if (strcmp(type, "i8") == 0) {
        uint8_t val;
        val = atoi((const char *)value);
        if (val == 0 && *value != 0x30) {
            ESP_LOGE(TAG, "Invalid value");
            return -1;
        }
        va_nvs_set_i8(variable, val);
    } else if (strcmp(type, "u16") == 0) {
        uint16_t val;
        val = (uint16_t)atoi((const char *)value);
        if (val == 0 && *value != 0x30) {
            ESP_LOGE(TAG, "Invalid value");
            return -1;
        }
        va_nvs_set_u16(variable, val);
    } else if (strcmp(type, "blob") == 0) {
        ESP_LOGE(TAG, "Not yet supported");
    } else {
        ESP_LOGE(TAG, "Incorrect argument");
    }
    return 0;
}

static int nvs_erase_cli_handler(int argc, char *argv[])
{
    /* Just to go to the next line */
    printf("\n");
    va_nvs_flash_erase();
    return 0;
}

static int reboot_cli_handler(int argc, char *argv[])
{
    /* Just to go to the next line */
    printf("\n");
    va_reset();
    esp_restart();
    return 0;
}

static int crash_cli_handler(int argc, char *argv[])
{
    /* Just to go to the next line */
    printf("\n");
    printf("%s: Crash the device by null dereference.\n", TAG);
    char *a = NULL;
    char b = *a;
    /* Adding this print so that compiler does not optimise 'b'. */
    printf("%c\n", b);
    return 0;
}

static esp_console_cmd_t diag_cmds[] = {
    {
        .command = "nvs-get",
        .help = "<variable> <string|blob|i8|u16>",
        .func = nvs_get_cli_handler,
    },
    {
        .command = "nvs-set",
        .help = "<variable> <string|i8|u16> <value>",
        .func = nvs_set_cli_handler,
    },
    {
        .command = "nvs-erase",
        .help = " ",
        .func = nvs_erase_cli_handler,
    },
    {
        .command = "reboot",
        .help = " ",
        .func = reboot_cli_handler,
    },
    {
        .command = "crash",
        .help = " ",
        .func = crash_cli_handler,
    }
};

int va_diag_register_cli()
{
    diag_register_cli();
    int cmds_num = sizeof(diag_cmds) / sizeof(esp_console_cmd_t);
    int i;
    for (i = 0; i < cmds_num; i++) {
        ESP_LOGI(TAG, "Registering command: %s", diag_cmds[i].command);
        esp_console_cmd_register(&diag_cmds[i]);
    }
    return 0;
}
