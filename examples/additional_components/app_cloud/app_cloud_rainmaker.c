// Copyright 2018 Espressif Systems (Shanghai) PTE LTD
// All rights reserved.

#include "sdkconfig.h"

#ifdef CONFIG_ALEXA_ENABLE_CLOUD

#include <string.h>
#include <esp_log.h>
#include <esp_console.h>

#include <esp_rmaker_core.h>
#include <esp_rmaker_standard_types.h>
#include <esp_rmaker_standard_params.h>
#include <esp_rmaker_standard_devices.h>
#include <esp_rmaker_ota.h>
#include <esp_rmaker_user_mapping.h>

#include "app_cloud.h"

static const char *TAG = "[app_cloud]";
esp_rmaker_node_t *node;

static int add_user_cli_handler(int argc, char *argv[])
{
    /* Just to go to the next line */
    printf("\n");
    if (argc != 3) {
        printf("%s: Incorrect arguments\n", TAG);
        return 0;
    }
    esp_rmaker_start_user_node_mapping(argv[1], argv[2]);
    return 0;
}

static esp_console_cmd_t cloud_cmds[] = {
    {
        .command = "add-user",
        .help = "add-user <user_id> <secret_key>",
        .func = add_user_cli_handler,
    }
};

static int app_cloud_register_cli()
{
    int cmds_num = sizeof(cloud_cmds) / sizeof(esp_console_cmd_t);
    int i;
    for (i = 0; i < cmds_num; i++) {
        ESP_LOGI(TAG, "Registering command: %s", cloud_cmds[i].command);
        esp_console_cmd_register(&cloud_cmds[i]);
    }
    return 0;
}

/* OTA event callback has not been implemented yet */
// static void app_cloud_ota_event_cb(cloud_ota_event_t event)
// {
//     switch (event) {
//         case CLOUD_OTA_START:
//             va_ui_set_state(VA_UI_OFF);
//             alexa_local_config_stop();
//             va_reset(false);
//             va_ui_set_state(VA_UI_OTA);
//             break;

//         case CLOUD_OTA_END:
//             va_ui_set_state(VA_UI_OFF);
//             break;

//         default:
//             break;
//     }
// }

void app_cloud_switch_driver_init()
{
    /* Initialize the device driver here */
}

void app_cloud_switch_state_update(bool state)
{
    /* Update the switch state here */
}

static esp_err_t write_cb(const esp_rmaker_device_t *device, const esp_rmaker_param_t *param,
            const esp_rmaker_param_val_t val, void *priv_data, esp_rmaker_write_ctx_t *ctx)
{
    if (strcmp(esp_rmaker_param_get_name(param), "power") == 0) {
        printf("%s: **************************************************************\n", TAG);
        printf("%s: ********************** %s turned %s **********************\n", TAG, esp_rmaker_device_get_name(device), val.val.b ? "ON" : "OFF");
        printf("%s: **************************************************************\n", TAG);
        app_cloud_switch_state_update(val.val.b);
        esp_rmaker_param_update_and_report(param, val);
    }
    return ESP_OK;
}

static void app_cloud_switch_init()
{
    app_cloud_switch_driver_init();

    esp_rmaker_device_t *switch_device = esp_rmaker_device_create("Switch", ESP_RMAKER_DEVICE_SWITCH, NULL);
    esp_rmaker_device_add_cb(switch_device, write_cb, NULL);

    esp_rmaker_device_add_param(switch_device, esp_rmaker_name_param_create("name", "Switch"));
    esp_rmaker_param_t *power_param = esp_rmaker_power_param_create("power", true);
    esp_rmaker_device_add_param(switch_device, power_param);
    esp_rmaker_device_assign_primary_param(switch_device, power_param);

    esp_rmaker_node_add_device(node, switch_device);
}

void app_cloud_init()
{
    app_cloud_register_cli();
    esp_rmaker_config_t rainmaker_cfg = {
        .enable_time_sync = false,
    };
    node = esp_rmaker_node_init(&rainmaker_cfg, "ESP RainMaker Device", "Switch");
    if (!node) {
        ESP_LOGE(TAG, "Could not initialise node. Aborting!!!");
        return;
    }

    /* Create a device and add the relevant parameters to it */
    app_cloud_switch_init();

    /* Enable OTA */
    esp_rmaker_ota_config_t ota_config = {
        .server_cert = (char *)ESP_RMAKER_OTA_DEFAULT_SERVER_CERT,
    };
    esp_rmaker_ota_enable(&ota_config, OTA_USING_TOPICS);

    /* Start the ESP RainMaker Agent */
    esp_rmaker_start();
}

#endif /* CONFIG_ALEXA_ENABLE_CLOUD */
