// Copyright 2018 Espressif Systems (Shanghai) PTE LTD
// All rights reserved.

#include <string.h>
#include <esp_err.h>
#include <esp_log.h>
#include <esp_wifi.h>
#include <esp_console.h>
#include <smart_home.h>
#include <alexa_smart_home.h>

static const char *TAG = "[app_smart_home]";

void app_device_driver_init()
{
    /* Initialise the device driver here */
}

void app_device_driver_update(const smart_home_device_t *device, const smart_home_param_t *param, const smart_home_param_val_t val)
{
    /* Update the device param's value here */
}

void app_smart_home_update_and_report_param(const smart_home_device_t *device, const smart_home_param_t *param, const smart_home_param_val_t val)
{
    /* Call this when state is changed locally */
    app_device_driver_update(device, param, val);
    smart_home_param_update_and_report(param, val);
}

static int app_smart_home_cli_handler(int argc, char *argv[])
{
    /* Just to go to the next line */
    printf("\n");
    if (argc != 4) {
        ESP_LOGE(TAG, "Incorrect arguments");
        return 0;
    }
    const char *device_name = argv[1];
    const char *param_name = argv[2];
    const char *value_str = argv[3];

    smart_home_device_t *device = smart_home_node_get_device_by_name(smart_home_get_node(), device_name);
    if (!device) {
        ESP_LOGE(TAG, "Device %s not found", device_name);
        return 0;
    }
    smart_home_param_t *param = smart_home_device_get_param_by_name(device, param_name);
    if (!param) {
        ESP_LOGE(TAG, "Param %s not found", param_name);
        return 0;
    }
    smart_home_param_val_t val = smart_home_param_get_val(param);
    smart_home_param_val_t new_val = {0};
    if (val.type == SMART_HOME_VAL_TYPE_BOOLEAN) {
        new_val = smart_home_bool(atoi(value_str));
    } else if (val.type == SMART_HOME_VAL_TYPE_INTEGER) {
        new_val = smart_home_int(atoi(value_str));
    } else if (val.type == SMART_HOME_VAL_TYPE_STRING) {
        new_val = smart_home_str(value_str);
    } else {
        ESP_LOGE(TAG, "Param value type not handled: %d", val.type);
        return 0;
    }

    app_smart_home_update_and_report_param(device, param, new_val);
    return 0;
}

static esp_console_cmd_t smart_home_cmds[] = {
    {
        .command = "smart-home",
        .help = "This can be used to simulate local control. Usage: smart-home <device_name> <param_name> <value>",
        .func = app_smart_home_cli_handler,
    },
};

static int app_smart_home_register_cli()
{
    int cmds_num = sizeof(smart_home_cmds) / sizeof(esp_console_cmd_t);
    int i;
    for (i = 0; i < cmds_num; i++) {
        ESP_LOGI(TAG, "Registering command: %s", smart_home_cmds[i].command);
        esp_console_cmd_register(&smart_home_cmds[i]);
    }
    return 0;
}

static esp_err_t write_cb(const smart_home_device_t *device, const smart_home_param_t *param, const smart_home_param_val_t val, void *priv_data, smart_home_write_ctx_t *ctx)
{
    const char *device_name = smart_home_device_get_name(device);
    const char *param_name = smart_home_param_get_name(param);

    if (val.type == SMART_HOME_VAL_TYPE_BOOLEAN) {
        printf("%s: *************** %s's %s turned %s ***************\n", TAG, device_name, param_name, val.val.b ? "ON" : "OFF");
    } else if (val.type == SMART_HOME_VAL_TYPE_INTEGER) {
        printf("%s: *************** %s's %s changed to %d ***************\n", TAG, device_name, param_name, val.val.i);
    } else if (val.type == SMART_HOME_VAL_TYPE_STRING) {
        printf("%s: *************** %s's %s changed to %s ***************\n", TAG, device_name, param_name, val.val.s);
    } else {
        ESP_LOGE(TAG, "Param value type not handled: %d", val.type);
    }

    app_device_driver_update(device, param, val);
    smart_home_param_update(param, val);
    return ESP_OK;
}

esp_err_t app_smart_home_init()
{
    /* Initialise device drivers */
    app_device_driver_init();

    /* Add CLI for local control and testing */
    app_smart_home_register_cli();

    /* Initialise smart_home */
    smart_home_init();
    const smart_home_node_t *node = smart_home_get_node();

    /* Add device */
    smart_home_device_t *device = smart_home_device_create("Light", alexa_smart_home_get_device_type_str(LIGHT), NULL);
    smart_home_device_add_cb(device, write_cb, NULL);
    smart_home_node_add_device(node, device);

    /* Add device info. All of these are required */
    static char mac_str[13] = {0};
    uint8_t mac_int[6] = {0};
    esp_wifi_get_mac(ESP_IF_WIFI_STA, mac_int);
    snprintf(mac_str, sizeof(mac_str), "%02x%02x%02x%02x%02x%02x", mac_int[0], mac_int[1], mac_int[2], mac_int[3], mac_int[4], mac_int[5]);

    alexa_smart_home_device_info_t device_info = {
        .device_serial_num = mac_str,
        .manufacturer_name = "Espressif",
        .device_description = "Alexa Device",
        .model_name = "ESP32",
        .product_id = CONFIG_ALEXA_PRODUCT_ID,
    };
    alexa_smart_home_device_add_info(device, &device_info);

    /* Add device parameters */
    smart_home_param_t *power_param = smart_home_param_create("Power", SMART_HOME_PARAM_POWER, smart_home_bool(true), SMART_HOME_PROP_FLAG_READ | SMART_HOME_PROP_FLAG_WRITE | SMART_HOME_PROP_FLAG_PERSIST);
    smart_home_device_add_param(device, power_param);

    smart_home_param_t *brightness_param = smart_home_param_create("Brightness", SMART_HOME_PARAM_RANGE, smart_home_int(100), SMART_HOME_PROP_FLAG_READ | SMART_HOME_PROP_FLAG_WRITE | SMART_HOME_PROP_FLAG_PERSIST);
    smart_home_param_add_bounds(brightness_param, smart_home_int(0), smart_home_int(100), smart_home_int(1));
    smart_home_device_add_param(device, brightness_param);

    return ESP_OK;
}
