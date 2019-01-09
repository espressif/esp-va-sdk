# Introduction
Google Voice Assistant(GVA) is Google's version of a personal voice assistant. GVA is multilingual and allows users to converse in their preferred language. Apart from general queries, it allows users to check on the traffic conditions, emails, weather conditions and much more.

# Project Setup
* Before proceeding with this section, make sure you have read and followed `README-Getting-Started.md`.
* Follow steps specified in this [link](https://developers.google.com/assistant/sdk/guides/library/python/embed/config-dev-project-and-account) and execute the following sections:
  * Configure an Actions Console project
  * Set activity controls for your account
  * Register the Device Model using the registration UI
  * Generate credentials

# Device Configuration
* In the project setup steps above, you would also have generated credentials to be configured in the device.
* Once you download credentials.json, you can use below commands on device console to set client ID, client secret and refresh token on the device.
```
[Enter]
>> nvs-set avs refreshToken string <refresh_token_from_credentials.json>
>> nvs-set avs clientId string <client_id_from_credentials.json>
>> nvs-set avs clientSecret string <client_secret_from_credentials.json>
```
* Use below CLI command to configure device's station interface
```
[Enter]
>> wifi-set <ssid> <passphrase>
```

# Demo
* Once the board successfully connects to the Wi-Fi network, you can use either use "Rec" button on the board or say "Alexa" to start a conversation. (The current example only supports the _Alexa_ wakeword. Support for other wakeword will be available soon.) For Tap-to-Talk, press and release the button and speak.
* You can connect any external speaker/headphone with 3.5mm connector to PHONE JACK to listen to responses.
* you can now ask any command like:
    * tell me a joke
    * how is the weather?
    * will it rain today?
    * Sing a song
    * Set volume level to 7
* Press and Hold "Mode" button for 3 seconds to reset the board to factory settings
* Assistant's language can be changed by setting appropriate code string va_cfg->device_config.device_language in app_main.c. List of valid codes strings can be found [here](https://developers.google.com/actions/localization/languages-locales).
