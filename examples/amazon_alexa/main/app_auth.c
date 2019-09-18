// Copyright 2018 Espressif Systems (Shanghai) PTE LTD
// All rights reserved.

#include <esp_console.h>
#include <esp_log.h>

#include <voice_assistant.h>
#include <alexa.h>

static char *TAG = "[app-auth]";

static int app_auth_sign_in_handler(int argc, char *argv[])
{
    /* Just to go to the next line */
    printf("\n");
    if (argc != 4) {
        ESP_LOGE(TAG, "Invalid parameters");
        return -1;
    }

    auth_delegate_config_t cfg = {0};
    cfg.type = auth_type_comp_app;
    cfg.u.comp_app.redirect_uri = argv[1];
    cfg.u.comp_app.auth_code = argv[2];
    cfg.u.comp_app.client_id = argv[3];
    cfg.u.comp_app.code_verifier = "abcd1234";
    alexa_auth_delegate_signin(&cfg);
    return 0;
}

static int app_auth_sign_out_handler(int argc, char *argv[])
{
    /* Just to go to the next line */
    printf("\n");
    alexa_auth_delegate_signout();
    return 0;
}

static esp_console_cmd_t auth_cmds[] = {
    {
        .command = "sign-in",
        .help = "sign-in <redirect uri> <auth code> <client ID>",
        .func = app_auth_sign_in_handler,
    },
    {
        .command = "sign-out",
        .help = "sign-out",
        .func = app_auth_sign_out_handler,
    },
};

int app_auth_register_cli()
{
    int cmds_num = sizeof(auth_cmds) / sizeof(esp_console_cmd_t);
    int i;
    for (i = 0; i < cmds_num; i++) {
        ESP_LOGI(TAG, "Registering command: %s", auth_cmds[i].command);
        esp_console_cmd_register(&auth_cmds[i]);
    }
    return 0;
}
