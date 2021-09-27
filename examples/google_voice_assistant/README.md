# Table Of Contents

* [1. Project Setup](#1-project-setup)
    * [1.1 Google Cloud Setup](#11-google-cloud-setup)
    * [1.2 Device Configuration](#12-device-configuration)
    * [1.3 Additional notes](#13-additional-notes)
* [2. Device Performance](#2-device-performance)
    * [2.1 CPU and Memory usage](#21-cpu-and-memory-usage)


# 1. Project Setup

## 1.1 Google Cloud Setup

*   Follow the steps specified in this [link](https://developers.google.com/assistant/sdk/guides/service/python/embed/config-dev-project-and-account) and execute the following sections:
    *   Configure an Actions Console project
    *   Set activity controls for your account
    *   Register the Device Model using the registration UI
    *   Download credentials

## 1.2 Device Configuration

Instead of configuration through the ESP Alexa phone app (it currently only supports Amazon sign-in), follow this to configure the device.

*   After the above steps you should have a file with the name client_secret_<client_id>.json. Now execute the first 3 steps in the section `Get an access token` in this [link](https://developers.google.com/assistant/sdk/reference/device-registration/register-device-manual#get-access-token).
*   You should now have a json with the `client_id`, `client_secret` and the `refresh_token` needed for the `Device Configuration`. Add this json to a file credentials.json, for future use.

*   Modify the example application (app_main.c) provided in this SDK, to add the `Model ID` (of your project) and a unique `Device ID` in the `device_model` and `device_id` members of `device_config` before making a call to `gva_init()`.
*   Build and flash the firmware again.
*   Use the following commands on device console to set client ID, client secret and refresh token, from credentials.json, on the device.
*   Make sure to enter the nvs-set commands first and then the wifi-set command.
```
[Enter]
>> nvs-set refreshToken string <refresh_token_from_json>
>> nvs-set clientId string <client_id_from_json>
>> nvs-set clientSecret string <client_secret_from_json>
```
*   Use below CLI command to configure device's station interface
```
[Enter]
>> wifi-set <ssid> <passphrase>
```

## 1.3 Additional notes

*   Assistant's language can be changed by setting appropriate code string va_cfg->device_config.device_language in app_main.c. List of valid code strings can be found [here](https://developers.google.com/actions/localization/languages-locales).
*   Wake-word is not available for GVA.


# 2. Device Performance

## 2.1 CPU and Memory usage

The following is the CPU and Memory Usage.

|                                       |Up And Running |Normal Queries |Music          |
|:-                                     |:-:            |:-:            |:-:            |
|**CPU Usage**                          |-              |-              |-              |
|**Free Internal Memory (328KB DRAM)**  |141KB          |138KB          |-              |
|**Free External Memory (4MB PSRAM)**   |2.75MB         |2.55MB         |-              |

**Flash Usage**: Firmware binary size: 1.9MB

This should give you a good idea about the amount of CPU and free memory that is available for you to run your application's code. For the free memory, a minimum of 50KB of free internal memory is required for healthy operation.
