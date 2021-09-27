# Table Of Contents

* [1. Project Setup](#1-project-setup)
    * [1.1 Google Cloud Setup](#11-google-cloud-setup)
    * [1.2 Device Configuration](#12-device-configuration)
    * [1.3 Additional notes](#13-additional-notes)
* [2. Device Performance](#2-device-performance)
    * [2.1 CPU and Memory usage](#21-cpu-and-memory-usage)


# 1. Project Setup

## 1.1 Google Cloud Setup

Before proceeding with device configuration, make sure you have read and followed the top-level Readme.

You will have to create a Dialogflow account and setup a Dialogflow agent in the cloud. This agent configuration is where you will specify what conversations will you be supporting.
*   Follow this [link](https://dialogflow.com/docs/getting-started) to get started.
*   Create your own agent
    *   You can add intents, entities, actions and parameters as per your agent's requirements.
    *   Build, test your agent and validate the responses using the console on Dialogflow.
*   Optionally, you can add an existing sample agent from [here](https://dialogflow.com/docs/samples) to your Dialogflow account and use the same.
Note:
*   Make sure that "Set this intent as end of conversation" is enabled under "Responses" tab in each intent of your Dialogflow agent so that the device can use this information to close the interaction with user

## 1.2 Device Configuration

Instead of configuration through the ESP Alexa phone app (it currently only supports Amazon sign-in), follow this to configure the device.

*   Your device needs to be configured with the correct credentials to talk to the above project.
*   Navigate to this [link](https://console.cloud.google.com/apis/dashboard)
    *   Select the agent created above as the project
    *   Go to Credentials section (on the left)
        *   Credentials -> Create credentials -> OAuth client ID -> Other
        *   OAuth consent screen -> Fill in the Support email (and other details as required) -> Save
        *   Credentials -> OAuth 2.0 client IDs -> Download the created OAuth client ID file (`client_secret_<client-id>.json`)
*   Follow the steps specified in this [link](https://developers.google.com/assistant/sdk/guides/library/python/embed/install-sample#generate_credentials)
    *   While using this step, use the following command instead of the one specified:
```
google-oauthlib-tool --scope https://www.googleapis.com/auth/cloud-platform \
    --save --headless --client-secrets /path/to/client_secret_<client-id>.json
```
*   You should now have a credentials.json file with the `client_id`, `client_secret` and the `refresh_token` needed for the `Device Configuration`. Copy this file for future use.

*   Modify the example application (app_main.c) provided in this SDK, to add the project ID (from `client_secret_<client-id>.json` file) in the `project_name` member of `device_config` before making a call to `dialogflow_init()`
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

*   Assistant's language can be changed by setting appropriate code string va_cfg->device_config.device_language in app_main.c. List of valid code strings can be found [here](https://dialogflow.com/docs/reference/language).
*   Wake-word is not available for Dialogflow.


# 2. Device Performance

## 2.1 CPU and Memory usage

The following is the CPU and Memory Usage.

|                                       |Up And Running |Normal Queries |Music          |
|:-                                     |:-:            |:-:            |:-:            |
|**CPU Usage**                          |-              |-              |-              |
|**Free Internal Memory (328KB DRAM)**  |141KB          |139KB          |-              |
|**Free External Memory (4MB PSRAM)**   |2.75MB         |2.75MB         |-              |

**Flash Usage**: Firmware binary size: 2.0MB

This should give you a good idea about the amount of CPU and free memory that is available for you to run your application's code. For the free memory, a minimum of 50KB of free internal memory is required for healthy operation.
