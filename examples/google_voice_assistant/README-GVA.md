# Introduction
Google Voice Assistant(GVA) is Google's version of a personal voice assistant. GVA is multilingual and allows users to converse in their preferred language. Apart from general queries, it allows users to check on the traffic conditions, emails, weather conditions and much more.

# Project Setup
* Before proceeding with device configuration, make sure you have read and followed the [Getting Started Guide](../../README-Getting-Started.md).
* Follow the steps specified in this [link](https://developers.google.com/assistant/sdk/guides/library/python/embed/config-dev-project-and-account) and execute the following sections:
  * Configure an Actions Console project
  * Set activity controls for your account
  * Register the Device Model using the registration UI
  * Generate credentials

# Device Configuration
* Modify the example application (app_main.c) provided in this SDK, to add the `Model ID` and `Device ID` (of your project) in the `device_model` and `device_id` members of `device_config` before making a call to `gva_init()`
* Build and flash the firmware as instructed in the [Getting Started Guide](../../README-Getting-Started.md).
* In the project setup steps above, you would also have generated credentials to be configured in the device.
* Once you download credentials.json, you can use the following commands on device console to set client ID, client secret and refresh token on the device.
* Make sure to enter the nvs-set commands first and then the wifi-set command.
```
[Enter]
>> nvs-set refreshToken string <refresh_token_from_credentials.json>
>> nvs-set clientId string <client_id_from_credentials.json>
>> nvs-set clientSecret string <client_secret_from_credentials.json>
```
* Use below CLI command to configure device's station interface
```
[Enter]
>> wifi-set <ssid> <passphrase>
```

# Demo
* Once the board successfully connects to the Wi-Fi network, you can use the "Rec" button on the board to start a conversation. (Support for wakeword will be available soon.) For Tap-to-Talk, press and release the button and speak.
* You can connect any external speaker/headphone with 3.5mm connector to PHONE JACK to listen to responses.
* You can now ask any command like:
    * Tell me a joke
    * How is the weather?
    * Will it rain today?
    * Sing a song
    * Set volume to 7
* Press and Hold "Mode" button for 3 seconds to reset the board to factory settings
* Assistant's language can be changed by setting appropriate code string va_cfg->device_config.device_language in app_main.c. List of valid codes strings can be found [here](https://developers.google.com/actions/localization/languages-locales).
