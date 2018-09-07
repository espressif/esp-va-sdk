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
#include <esp_heap_trace.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <mem_utils.h>
#include <avs_nvs_utils.h>
#include <string.h>
#include <nvs.h>
#include <scli.h>

static const char *TAG = "diag";
static int task_dump_cli_handler(int argc, char *argv[])
{
    int num_of_tasks = uxTaskGetNumberOfTasks();
    TaskStatus_t *task_array = mem_alloc(num_of_tasks * sizeof( TaskStatus_t ), EXTERNAL);
    if (!task_array) {
        ESP_LOGE(TAG, "Memory not allocated for task list.");
    }
    num_of_tasks = uxTaskGetSystemState(task_array, num_of_tasks, NULL);
    printf("Name\tNumber\tPriority\tStackWaterMark\n");
    for (int i = 0; i < num_of_tasks; i++) {
        printf("%16s\t%d\t%d\t%d\n",
               task_array[i].pcTaskName,
               task_array[i].xTaskNumber,
               task_array[i].uxCurrentPriority,
               task_array[i].usStackHighWaterMark);
    }
    mem_free(task_array);
    return 0;
}

static int mem_dump_cli_handler(int argc, char *argv[])
{
    printf("Description\tInternal\tSPIRAM\n");
    printf("Current Free Memory\t%d\t%d\n",
           heap_caps_get_free_size(MALLOC_CAP_8BIT) - heap_caps_get_free_size(MALLOC_CAP_SPIRAM),
           heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
    printf("Largest Free Block\t%d\t%d\n",
           heap_caps_get_largest_free_block(MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL),
           heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM));
    printf("Min. Ever Free Size\t%d\t%d\n",
           heap_caps_get_minimum_free_size(MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL),
           heap_caps_get_minimum_free_size(MALLOC_CAP_SPIRAM));
    return 0;
}

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
    const char *namespace = argv[1];
    const char *variable = argv[2];
    const char *type = argv[3];
    size_t val_length = 0;
    if (argc != 4) {
        printf("Incorrect arguments\n");
        return 0;
    }
    if (strcmp(type, "string") == 0) {
        avs_nvs_get_str(namespace, variable, NULL, &val_length);
        if (val_length == 0) {
            printf("Variable with 0 length\n");
            return 0;
        } else {
            char *value = mem_alloc(val_length, EXTERNAL);
            if (value == NULL) {
                printf("Memory allocation failed");
                return 0;
            }
            avs_nvs_get_str(namespace, variable, value, &val_length);
            printf("%s\n", value);
            mem_free(value);
        }
    } else if (strcmp(type, "blob") == 0) {
        avs_nvs_get_blob(namespace, variable, NULL, &val_length);
        if (val_length == 0) {
            printf("Variable with 0 length\n");
            return 0;
        } else {
            uint8_t *value = mem_alloc(val_length, EXTERNAL);
            if (value == NULL) {
                printf("Memory allocation failed");
                return 0;
            }
            avs_nvs_get_blob(namespace, variable, value, &val_length);
            hex_dump(value, val_length);
            mem_free(value);
        }
    } else {
        printf("Incorrect argument\n");
    }
    return 0;
}

static int nvs_set_cli_handler(int argc, char *argv[])
{
    const char *namespace = argv[1];
    const char *variable = argv[2];
    const char *type = argv[3];
    char *value = argv[4];

    if (argc != 5) {
        printf("Incorrect arguments\n");
        return 0;
    }
    if (strcmp(type, "string") == 0) {
        avs_nvs_set_str(namespace, variable, value);
    } else if (strcmp(type, "blob") == 0) {
        printf("Not yet supported\n");
    } else {
        printf("Incorrect argument\n");
    }
    return 0;
}

static int heap_trace_records;
static heap_trace_record_t *heap_trace_records_buf;
static void cli_heap_trace_start()
{
    if (!heap_trace_records_buf) {
        heap_trace_records_buf = mem_alloc(heap_trace_records * sizeof(heap_trace_record_t), INTERNAL);
        if (!heap_trace_records_buf) {
            printf("Failed to allocate records buffer\n");
            return;
        }
        if (heap_trace_init_standalone(heap_trace_records_buf, heap_trace_records) != ESP_OK) {
            printf("Failed to initialise tracing\n");
            goto error1;
        }
    }
    if (heap_trace_start(HEAP_TRACE_LEAKS) != ESP_OK) {
        printf("Failed to start heap trace\n");
        goto error2;
    }
    return;
 error2:
    heap_trace_init_standalone(NULL, 0);
 error1:
    mem_free(heap_trace_records_buf);
    heap_trace_records_buf = NULL;
    return;
}

static void cli_heap_trace_stop()
{
    if (!heap_trace_records_buf) {
        printf("Tracing not started?\n");
        return;
    }
    heap_trace_stop();
    heap_trace_dump();
    heap_trace_init_standalone(NULL, 0);
    mem_free(heap_trace_records_buf);
    heap_trace_records_buf = NULL;
}

static int heap_trace_cli_handler(int argc, char *argv[])
{
#ifndef CONFIG_HEAP_TRACING
    printf("To use this utility enable: Component config --> Heap memory debugging --> Enable heap tracing\n");
    return 0;
#endif
    if (argc < 2) {
        printf("Incorrect arguments\n");
        return 0;
    }
    if (strcmp(argv[1], "start") == 0) {
#define DEFAULT_HEAP_TRACE_RECORDS 100
        if (argc != 3) {
            heap_trace_records = DEFAULT_HEAP_TRACE_RECORDS;
        } else {
            heap_trace_records = atoi(argv[2]);
        }
        printf("Using a buffer to trace %d records\n", heap_trace_records);
        cli_heap_trace_start();
    } else if (strcmp(argv[1], "stop") == 0) {
        cli_heap_trace_stop();
    } else {
        printf("Invalid argument:%s:\n", argv[1]);
    }
    return 0;
}

static esp_console_cmd_t diag_cmds[] = {
    {
        .command = "mem-dump",
        .help = "",
        .func = mem_dump_cli_handler,
    },
    {
        .command = "task-dump",
        .help = "",
        .func = task_dump_cli_handler,
    },
    {
        .command = "nvs-get",
        .help = "<namespace> <variable> <string|blob>",
        .func = nvs_get_cli_handler,
    },
    {
        .command = "nvs-set",
        .help = "<namespace> <variable> <string|blob> <value>",
        .func = nvs_set_cli_handler,
    },
    {
        .command = "heap-trace",
        .help = "<start|stop> [trace-buf-size]",
        .func = heap_trace_cli_handler,
    },
};

int diag_register_cli()
{
    int cmds_num = sizeof(diag_cmds) / sizeof(esp_console_cmd_t);
    int i;
    for (i = 0; i < cmds_num; i++) {
        printf("Registering command: %s \n", diag_cmds[i].command);
        esp_console_cmd_register(&diag_cmds[i]);
    }
    return 0;
}

