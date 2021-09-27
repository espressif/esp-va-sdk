# Table Of Contents

* [0. Important Note](#0-important-note)
* [1. Introduction](#1-introduction)
    * [1.1 Solution Architecture](#11-solution-architecture)
    * [1.2 The Software](#12-the-software)
    * [1.3 Voice Assistants](#13-voice-assistants)
        * [1.3.1 Alexa Voice Service (AVS)](#131-alexa-voice-service-avs)
        * [1.3.2 AVS for IoT (AFI)](#132-avs-for-iot-afi)
        * [1.3.3 Google Voice Assistant (GVA)](#133-google-voice-assistant-gva)
        * [1.3.4 Google Dialogflow](#134-google-dialogflow)
    * [1.4 The ESP32-Vaquita-DSPG Development Board](#14-the-esp32-vaquita-dspg-development-board)
        * [1.4.1 Buttons](#141-buttons)
* [2. Development Setup](#2-development-setup)
    * [2.1 Host Setup](#21-host-setup)
    * [2.2 Getting the Repositories](#22-getting-the-repositories)
    * [2.3 Building the Firmware](#23-building-the-firmware)
    * [2.4 Flashing the Firmware](#24-flashing-the-firmware)
* [3. Additional Setup](#3-additional-setup)
* [4. Device Provisioning](#4-device-provisioning)
    * [4.1 Configuration Steps](#41-configuration-steps)
    * [4.2 Additional Device Settings](#42-additional-device-settings)
* [5. Customising for your Board](#5-customising-for-your-board)
* [6. Integrating other components](#6-integrating-other-components)
    * [6.1 ESP RainMaker](#61-esp-rainmaker)
        * [6.1.1 Environment Setup](#611-environment-setup)
        * [6.1.2 Device Provisioning](#612-device-provisioning)
        * [6.1.3 Customisation](#613-customisation)
    * [6.2 Smart Home](#62-smart-home)
        * [6.2.1 Usage](#621-usage)
        * [6.2.2 Customisation](#622-customisation)
    * [6.3 Audio Player](#63-audio-player)
        * [6.3.1 Enabling Custom Player](#631-enabling-custom-player)
        * [6.3.2 Customisation](#632-customisation)
    * [6.4 Equalizer](#64-equalizer)
        * [6.4.1 Enabling Equalizer](#641-enabling-equalizer)
* [7. Production Considerations](#7-production-considerations)
    * [7.1 Over-the-air Updates (OTA)](#71-over-the-air-updates-ota)
    * [7.2 Manufacturing](#72-manufacturing)
        * [7.2.1 Mass Manufacturing Utility](#721-mass-manufacturing-utility)
        * [7.2.2 Pre-Provisioned Modules](#722-pre-provisioned-modules)
    * [7.3 Security](#73-security)
        * [7.3.1 Secure Boot](#731-secure-boot)
        * [7.3.2 Flash Encryption](#732-flash-encryption)
        * [7.3.3 NVS Encryption](#733-nvs-encryption)
* [A1 Appendix FAQs](#a1-appendix-faqs)
    * [A1.1 Compilation errors](#a11-compilation-errors)
    * [A1.2 Device setup using the Mobile app](#a12-device-setup-using-the-mobile-app)
    * [A1.3 Device crashing](#a13-device-crashing)
    * [A1.4 Device not crashed but not responding](#a14-device-not-crashed-but-not-responding)


# 0. Important Note

The Wake-word ("Alexa") recognition software that is part of the [GitHub repository](https://github.com/espressif/esp-va-sdk) is for evaluation only. Please contact sales@espressif.com for production-ready Wake-word recognition DSP Firmware that is available from our DSP partners.

Please refer to [Changelog](CHANGELOG.md) to track release changes and known-issues.

# 1. Introduction

Espressif's Voice Assistant SDK allows customers to build Alexa and Google built-in smart devices. This allows customers to leverage Espressif's rich IoT device development capability while simultaneously incorporating voice enablement in these devices.

## 1.1 Solution Architecture

The typical solution architecture of the product is shown as below. 

<center>
    <img src="https://github.com/espressif/esp-va-sdk/wiki/va_images/esp_va_sdk_solution_architecture.png" alt="Solution Architecture Block Diagram" title="Solution Architecture Block Diagram" width="800" />
</center>

The following are the relevant blocks for the solution:

1.  **ESP32**: This is the primary microcontroller that controls the operations of the product.
    1.  **Voice Assistant Client**: It runs the Voice Assistant client that manages the audio communication with the Voice Assistant's cloud. ESP32 is also responsible for any state management, audio encode/decode operations.
    2.  **IoT Device**: It also runs the software that interfaces with your peripherals providing the smart-device functionality that you wish to expose.
2.  **DSP**: The DSP typically performs the Noise Reduction (NR), Acoustic Echo Cancellation (AEC) and run the Wake-Word Engine (WWE). The DSP is interfaced with the Mic Array for the audio input. And it subsequently interacts with the ESP32 for relaying the audio input.
3.  **Codec**: The playback data is received by the Codec which it subsequently sends to the speaker.

## 1.2 The Software

The software that is part of this SDK is sulf-sufficient to provide a full voice-assistant capability for your device. Typically, as a device manufacturer, you may want to customise certain configurations of this software. If you also wish to expose some additional functionality beyond voice-assistant (switch, fan, water purifier, etc), you will also have to write the device drivers for controlling this.

<center>
    <img src="https://github.com/espressif/esp-va-sdk/wiki/va_images/esp_va_sdk_software_components.png" alt="Software Components Block Diagram" title="Software Components Block Diagram" width="900" />
</center>

The above block diagram indicates the various components of the Voice Assistant SDK.

## 1.3 Voice Assistants

### 1.3.1 Alexa Voice Service (AVS)

Alexa is Amazon's personal virtual assistant which listens to user's voice commands and responds with appropriate answers. Apart from conversing with the user, Alexa lets you play music from a variety of music streaming services. Alexa also helps you manage to-do lists and allows for voice-assisted shopping from Amazon.

This particular flavour of Alexa is helpful when you are building a 'speaker' class of device with esp-va-sdk.

AVS also supports playing music through Bluetooth working in conjunction with music from Alexa.

### 1.3.2 AVS for IoT (AFI)

AVS for IoT (AFI) is also known as AVS integrated with AWS IoT (AIA). The Amazon cloud does the audio decoding from various sources and sends them to the device. This reduces the processing and memory usage on the device. This flavour of Alexa would be particularly helpful if you are building a voice-assistant end-device that is not just a speaker, but additionally something else (switch, fan, water purifier, etc).

AIA also supports AVS smart home integration. Let's consider you are a alexa-enabled light bulb. Smart Home integration implies that if you say a query like 'Alexa, switch off the light' (where the light is your own device), then the AIA cloud service will decode this into actionable data that comes back to your device, and you can parse it and execute the action (in this case switching off the light).

### 1.3.3 Google Voice Assistant (GVA)

GVA is Google's version of a personal voice assistant. It is multilingual and allows users to converse in their preferred language. Apart from general queries, it allows users to check on the traffic conditions, emails, weather conditions and much more.

Note that this SDK only includes a Proof-of-Concept (PoC) implementation for GVA. This is not recommended for production.

### 1.3.4 Google Dialogflow

Dialogflow (previously known as API.AI) is a voice enabled conversational interface from Google.
It enables IoT users to include natural language user interface in their applications, services and devices.

The advantages of Dialogflow wrt voice assistants are, less complexity, pay as you go pricing, custom wakeword allowed and no certification hassles.

Unlike other voice assistants, Dialogflow let's you configure every step of the conversation, and it won't answer other trivia/questions like voice assistants typically do. For e.g. A Dialogflow agent for Laundry project will provide information only about the configurable parameters of the laundry (like state, temperature, wash cycle, etc.).

The implementation here facilitates the audio communication of ESP32 with a Google Dialogflow agent using its v2beta1 gRPC APIs.

## 1.4 The ESP32-Vaquita-DSPG Development Board

The ESP32-Vaquita-DSPG Development Board is Amazon-certified for Alexa functionality. The solution consists of the ESP32 micro-controller paired with DSP-G's DBMD5P SoC. The ESP32 provides the Wi-Fi connectivity and implements the Voice Assistant client, the DBMD5P runs the acoustic front-end and the wake-word engine.

The following is a picture of the ESP32-DBMD5P Dev Kit.

<center>
    <img src="https://github.com/espressif/esp-va-sdk/wiki/va_images/esp32_vaquita_dspg_base_board.png" alt="ESP32-Vaquita-DSPG Board" title="ESP32-Vaquita-DSPG Board" width="500" />
    <img src="https://github.com/espressif/esp-va-sdk/wiki/va_images/esp32_vaquita_dspg_mic_board.png" alt="ESP32-Vaquita-DSPG Board" title="ESP32-Vaquita-DSPG Board" width="500" />
</center>


The kit has the following contents:

*   Dev Kit
*   ESP32 as the host micro-controller
*   DBMD5P running the acoustic frontend and wake-word engine
*   2 Push Buttons
*   5 RGB LEDs (Linear)
*   2-mic Mic Array board

### 1.4.1 Buttons

**Push-to-Talk (Button1)**: Press this button to initiate conversation with the Assistant without saying the wake-word.

**Microphone Mute (Button2)**: Press this button to disable/enable microphone on the device.

**Reset to Factory**: This wipes all the settings (Network configuration, Alexa/Google login credentials) from the device, and the device goes back to default factory settings. Press and hold **Mute + Push-To-Talk** buttons together for 10 seconds. (until you see orange LEDs)

**Reset Wi-Fi Configuration**: Use this to switch the device into Wi-Fi change mode. The device will stay in this mode for 3 minutes, after which it will go back to the normal mode of operation. You can use the phone apps, within this time-frame, to re-configure the Wi-Fi credentials of the Dev Kit. The new Wi-Fi credentials will overwrite the previous Wi-Fi configuration. Press and hold **Push-To-Talk** button for 5 seconds. (until you see orange LEDs)

**Note**: All the supported boards are mentioned here: [audio_board](https://github.com/espressif/esp-va-sdk/tree/master/components/audio_hal/audio_board/)

# 2. Development Setup

This sections talks about setting up your development host, fetching the git repositories, and instructions for build and flash.

## 2.1 Host Setup

You should install drivers and support packages for your development host. Windows, Linux and Mac OS-X, are supported development hosts. Please see [Get Started](https://docs.espressif.com/projects/esp-idf/en/release-v4.2/get-started/index.html) for the host setup instructions.

## 2.2 Getting the Repositories

```
$ git clone --recursive https://github.com/espressif/esp-idf.git

$ cd esp-idf; git checkout release/v4.2; git submodule init; git submodule update --init --recursive;

$ ./install.sh

$ cd ..

$ git clone https://github.com/espressif/esp-va-sdk.git
```

## 2.3 Building the Firmware

```
$ cd esp-va-sdk/examples/amazon_aia/ (for AIA or amazon_alexa for AVS or google_voice_assistant or google_dialogflow)

$ export ESPPORT=/dev/cu.SLAB_USBtoUART (or /dev/ttyUSB0 or /dev/ttyUSB1 on Linux or COMxx on MinGW)

$ export IDF_PATH=/path/to/esp-idf

$ . $IDF_PATH/export.sh
```

Set audio_board path. e.g. For ESP32-Vaquita-DSPG:

```
$ export AUDIO_BOARD_PATH=/path/to/esp-va-sdk/components/audio_hal/audio_board/audio_board_vaquita_dspg
```

Menuconfig changes:
```
Do this change only if you are using ESP32-WROVER-E module:
$ idf.py menuconfig
-> Component config -> ESP32-specific -> Minimum Supported ESP32 Revision -> change Rev_0 to Rev_3

Do these changes only if your board uses spiffs partition for storing the dsp firmware (Refer to the $AUDIO_BOARD_PATH/audio_board.cmake file):
$ idf.py menuconfig
-> Partition Table -> Custom partition CSV file -> change to partitions_spiffs.csv

Do these changes only if you are using ESP32 module with 4MB flash size (Refer to the $AUDIO_BOARD_PATH/audio_board.cmake file):
$ idf.py menuconfig
-> Serial flasher config -> Flash size -> change to 4MB
-> Partition Table -> Custom partition CSV file -> change to partitions_4mb_flash.csv
```

## 2.4 Flashing the Firmware

(When flashing the SDK for the first time, it is recommended to do `idf.py erase_flash` to wipe out entire flash and start out fresh.)

```
$ idf.py flash monitor
```

# 3. Additional Setup

The device would need additional configuration on the cloud as well as the device firmware for it to work. Check the README in the example directory for the voice assistant specific `Project Setup`.

# 4. Device Provisioning

For Google Voice Assistant and Google Dialogflow, please refer to the READMEs in the respective examples instead of the description that follows below.

The configuration step consists of (a) configuring the Wi-Fi network and (b) signing into your Alexa account and linking the device. Espressif has released the following phone applications that facilitate the same:

**iOS**: [iOS app](https://apps.apple.com/in/app/esp-alexa/id1464127534) <br>
**Android**: [Android app](https://play.google.com/store/apps/details?id=com.espressif.provbleavs)

Please install the relevant application on your phone before your proceed.

## 4.1 Configuration Steps

Here are the steps to configure the Dev Kit

*   On first boot-up, the Dev Kit is in configuration mode. This is indicated by Orange LED pattern. Please ensure that the LED pattern is seen as described above, before you proceed.
*   Launch the phone app.
*   Select the option *Add New Device*.

<center>
    <img src="https://github.com/espressif/esp-va-sdk/wiki/va_images/esp_alexa_app_home.png" alt="App Home" title="App Home" width="300" />
</center>

*   A list of devices that are in configuration mode is displayed. Note that the devices are discoverable over BLE (Bluetooth Low Energy). Please ensure that the phone app has the appropriate permissions to access Bluetooth (on Android the *Location* permission is also required for enabling Bluetooth).

<center>
    <img src="https://github.com/espressif/esp-va-sdk/wiki/va_images/esp_alexa_app_discover_devices.png" alt="App Discover Devices" title="App Discover Devices" width="300" />
</center>

*   Now you can sign-in to your Amazon Alexa account. If you have Amazon Shopping app installed on the same phone, app will automatically sign-in with the account the shopping app is signed in to. Otherwise it will open a login page on the phone's default browser. (It is recommended to install the Amazon Shopping app on your phone to avoid any other browser related errors.)

<center>
    <img src="https://github.com/espressif/esp-va-sdk/wiki/va_images/esp_alexa_app_sign_in.png" alt="App Sign-in" title="App Sign-in" width="300" />
</center>

*   You can now select the Wi-Fi network that the Dev Kit should connect with, and enter the credentials for this Wi-Fi network.

<center>
    <img src="https://github.com/espressif/esp-va-sdk/wiki/va_images/esp_alexa_app_wifi_scan_list.png" alt="App Scna List" title="App Scan List" width="300" />
    <img src="https://github.com/espressif/esp-va-sdk/wiki/va_images/esp_alexa_app_wifi_password.png" alt="App Wi-Fi Password" title="App Wi-Fi Password" width="300" />
</center>

*   On successful Wi-Fi connection, you will see a list of few of the voice queries that you can try with the Dev Kit.

<center>
    <img src="https://github.com/espressif/esp-va-sdk/wiki/va_images/esp_alexa_app_things_to_try.png" alt="App Things To Try" title="App Things To Try" width="300" />
</center>

*   You are now fully setup. You can now say "Alexa" followed by the query you wish to ask.

## 4.2 Additional Device Settings

Some device settings like Volume Control, Locale Change, etc. can also be controlled through the phone app.

*   Launch the phone app, select the option *Manage devices*.

<center>
    <img src="https://github.com/espressif/esp-va-sdk/wiki/va_images/esp_alexa_app_home.png" alt="App Home" title="App Home" width="300" />
</center>

*   Make sure you are connected to the same network as the device and also that SSDP packets can be sent on your network.
*   Now select your device from the list of devices for the device settings.


# 5. Customising for your Board

For integrating/customising your own board, refer to components/audio_hal/README.md

# 6. Integrating other components

## 6.1 ESP RainMaker

### 6.1.1 Environment Setup

Additional setup that needs to be done for integrating [ESP RainMaker](https://rainmaker.espressif.com/):

*   Get the repository:
    ```
    $ git clone --recursive https://github.com/espressif/esp-rainmaker.git
    ```
*   Setting cloud_agent:
    ```
    $ export CLOUD_AGENT_PATH=/path/to/esp-rainmaker
    ```
*   Menuconfig changes:
    ```
    $ idf.py menuconfig
    -> Voice Assistant Configuration -> Enable cloud support -> enable this
    ```

### 6.1.2 Device Provisioning

The combined app for ESP RainMaker + ESP Alexa is still under development. Till then, both the apps can be used separately for provisioning.

*   Open the ESP RainMaker app and sign-in.
*   Click on add device, scan the QR code and complete the Wi-Fi setup.
*   The app will verify the setup.

*   (For GVS and Dialogflow, refer to their respective READMEs for provisioning.)
*   Make sure you are connected to the same network as the device.   
*   Open the ESP Alexa app -> Manage Devices.
*   Find your device and sign-in into Alexa.

### 6.1.3 Customisation

To customise your own device, you can edit the file examples/additional_components/app_cloud/app_cloud_rainmaker.c. You can check the examples in ESP RainMaker for some more device examples.

## 6.2 Smart Home

This is only for AIA (AVS support will be added soon).

**Note:** There is a bug where the device name is being set as 'Demo Light' instead of what is being set by the device (default is 'Light').

One way to add the smart home functionality is to use [ESP RainMaker](#91-esp-rainmaker), and the other way is to use *examples/additional_components/app_smart_home*. This can be initialized in the appilication. Uncomment `app_smart_home_init()` in *app_main.c*.

### 6.2.1 Usage

Once provisioning is done and the device has booted up, the smart home feature of the device can be used via voice commands or through the Alexa app.

Example: By default, the device configured is a 'Light' with 'Power' and 'Brightness' functionalities. Voice commands like 'Turn on the Light' or 'Change Light Brightness to 50' can be used. In the Alexa app, this device will show up as 'Light' and the Power and Brightness can be controlled.

### 6.2.2 Customisation

To customise your own device, you can edit the file examples/additional_components/app_smart_home/app_smart_home.c. You can refer the files *components/voice_assistant/include/smart_home.h* and *components/voice_assistant/include/alexa_smart_home.h* for additional APIs.

A device can have the following types of capabilities/features/parameters:
*   Power: A device can only have a single power param.
*   Toggle: This can be used for params which can be toggled. Example: Turning on/off the swinging of the blades in an air conditioner.
*   Range: This can be used for params which can have a range of values. Example: Changing the brightness of a light.
*   Mode: This can be used for params which need to be selected from a pre-defined set of strings. Example: Selecting the modes of a washing machine.

## 6.3 Audio Player

The audio player (*components/voice_assistant/include/audio_player.h*) can be used to play custom audio files from any source (http url, local spiffs, etc.).

The focus management (what is currently being played) is already implemented internally by the SDK.

For AIA: Speech/Alert/Music from Alexa has higher priority than what is played via the audio_player. So for example, if custom music is being played via the audio_player, and a query is asked, then the music will be paused and the response from Alexa will be played. Once the response is over, the music will be resumed (unless already stopped). Basically, all Alexa audio gets priority over custom audio.

For AVS: Speech/Alert from Alexa has higher priority than what is played via the audio_player. So for example, if custom music is being played via the audio_player, and a query is asked, then the music will be paused and the response from Alexa will be played. Once the response is over, the music will be resumed (unless already stopped). Another example, if custom music is being played via audio_player, and a query is asked for playing music via the cloud, then the custom music will be stopped and the music from Alexa will take over. If Alexa music was playing and custom music is played, then Alexa music will stop and the custom music will take over. Basically, music has the same priority from whichever source it is being played from. All other Alexa audio gets priority over music.

For GVA and Dialogflow: Speech (Alerts and Music is not yet supported) from Google has higher priority than what is played via the audio_player. So for example, if custom music is being played via the audio_player, and a query is asked, then the music will be paused and the response from Alexa will be played. Once the response is over, the music will be resumed (unless already stopped). Basically, all Google audio gets priority over custom audio.

### 6.3.1 Enabling Custom Player

The *examples/additional_components/custom player* is an example using the audio_player. The default example of the custom player can play from http url and/or local spiffs and/or local sdcard but can be easily extended to play from any other source.

Easiest way to try custom player is using http url.

*   Include *custom_player.h* in the application and call *custom_player_init()* after the voice assistant early initialisation has been done.

When the application is now built and flashed on the device, the custom player will play the 3 files showing the usage of the audio_player.

### 6.3.2 Customisation

The default custom player just has a demo code which can be used as a reference to build your own player. The audio player, for now, just supports mp3 and aac audio formats for http urls and only mp3 audio format for local files.

## 6.4 Equalizer

This is only for AVS.

Equalizer lets you control the Bass, Mid-Range and Treble of the audio. You can use the following commands to get the values for the equalizer:
*   Set Treble to 3
*   Set Bass to -3
*   Reset Equalizer
*   Set Movie mode

The SDK will give a callback to the application (in equalizer.c) with the respective values for the equalizer. The application can then use these values and adjust the audio output.

### 6.4.1 Enabling Equalizer

To enable the equalizer along with Alexa:
*   Include *alexa_equalizer.h* in the application and call *alexa_equalizer_init()* before the voice assistant initialisation has been done.

# 7. Production Considerations

## 7.1 Over-the-air Updates (OTA)

ESP-IDF has a component for OTA from any URL. More information and details about implementing can be found here: [esp_https_ota](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/esp_https_ota.html#esp-https-ota).

## 7.2 Manufacturing

### 7.2.1 Mass Manufacturing Utility

The devices generally require unique IDs and certificates to connect to the cloud/server. For example, in AIA, AWS IoT operations require that all devices have a unique certificate and key pair programmed on each device which is used for authentication with the AWS IoT cloud service.

These are generally programmed in factory NVS partitions that are unique per device. ESP-IDF provides a utility to create instances of factory NVS partition images on a per-device basis for mass manufacturing purposes. The NVS partition images are created from CSV files containing user-provided configurations and values.

Details about using the mass manufacturing utility can be found here: [mass_manufacturing](https://docs.espressif.com/projects/esp-idf/en/latest/api-reference/storage/mass_mfg.html).

### 7.2.2 Pre-Provisioned Modules

ESP32 modules can be pre-flashed with the factory NVS partition during manufacturing itself and then be shipped to you. For example, in AIA, the device certificates are signed by your Certificate Authority (CA) and when you register this CA in your cloud, all the devices can connect to the cloud, out of the box.

This saves you the overhead of securely generating, encrypting and then programming the NVS partition into the device at your end. Pre-provisioning is an optional service which Espressif provides.

Please contact your Espressif contact person for more information.

## 7.3 Security

### 7.3.1 Secure Boot

Secure boot ensures that only trusted code runs on the device.

ESP32 supports RSA based secure boot scheme whereby the bootROM verifies the software boot loader for authenticity using the RSA algorithm. The verified software boot loader then checks the partition table and verifies the active application firmware and then boots it.

Details about implementing the secure boot can be found here: [secure_boot](https://docs.espressif.com/projects/esp-idf/en/latest/security/secure-boot.html).

### 7.3.2 Flash Encryption

Flash encryption prevents the plain-text reading of the flash contents.

ESP32 supports AES-256 based flash encryption scheme. The ESP32 flash controller has an ability to access the flash contents encrypted with a key and place them in the cache after decryption. It also has ability to allow to write the data to the flash by encrypting it. Both the read/write encryption operations happen transparently.

Details about implementing the flash encryption can be found here: [flash_encryption](https://docs.espressif.com/projects/esp-idf/en/latest/security/flash-encryption.html).

### 7.3.3 NVS Encryption

For the manufacturing data that needs to be stored on the device in the NVS format, ESP-IDF provides the NVS image creation utility which allows the encryption of NVS partition on the host using a randomly generated (per device unique) or pre-generated (common for a batch) NVS encryption key.

A separate flash partition is used for storing the NVS encryption keys. This flash partition is then encrypted using flash encryption. So, flash encryption becomes a mandatory feature to secure the NVS encryption keys.

Details about implementing the NVS encryption can be found here: [nvs_encryption](https://docs.espressif.com/projects/esp-idf/en/latest/api-reference/storage/nvs_flash.html#nvs-encryption).

# A1 Appendix FAQs

## A1.1 Compilation errors

I cannot build the application:

*   Make sure you are on the correct esp-idf branch. Run 'git submodule update ---init ---recursive' to make sure the submodules are at the correct heads
*   Make sure you have the correct AUDIO_BOARD_PATH selected for your board.
*   Delete the build/ directory and also sdkconfig and sdkconfig.old and then build again.
*   If you are still facing issues, reproduce the issue on the default example and then contact Espressif for help. Please make sure to share these:
    *   The esp-va-sdk and esp-idf branch you are using and the AUDIO_BOARD_PATH that you have set.
    *   The complete build logs.

## A1.2 Device setup using the Mobile app

I cannot *Add a new device* through the phone app:

*   If the device is not being shown while Adding a new device, make sure the Required permissions are given to the app. Also make sure that your Bluetooth is turned on.
*   Android typically requires the *Location* permission also for enabling Bluetooth.
*   If you are still facing issues, update the app to the latest version and try again.
*   Force closing the app and rebooting the device works in most cases if either of them have gone into an unknown state.
*   If you are still facing issues, reproduce the issue on the default example for the device and then contact Espressif for help. Make sure to share these:
    *   Screenshots of the mobile app where it is not working.
    *   Mobile App version.
    *   Mobile Phone model and the Android version or any skin it is running.
    *   Complete device logs taken over UART.
    *   The esp-va-sdk and esp-idf branch you are using and the AUDIO_BOARD_PATH that you have set.

I cannot *Manage device* through the phone app:

*   If the device is not being shown while Managing devices, make sure you are connected to the same network as the device.
*   If you are still facing issues, update the app to the latest version and try again.
*   Force closing the app and rebooting the device works in most cases if either of them have gone into an unknown state.
*   If you are still facing issues, reproduce the issue on the default example for the device and then contact Espressif for help. Make sure to share these:
    *   Screenshots of the mobile app where it is not working.
    *   Mobile App version.
    *   Mobile Phone model and the Android version or any skin it is running.
    *   Complete device logs taken over UART.
    *   The esp-va-sdk and esp-idf branch you are using and the AUDIO_BOARD_PATH that you have set.

## A1.3 Device crashing

My device is crashing:

*   Given the tight footprint requirements of the device, please make sure any issues in your code have been ruled out. If you believe the issue is with the Alexa SDK itself, please recreate the issue on the default example application (without any changes) and go through the following steps:
*   Make sure you are on the correct esp-idf branch. Run 'git submodule update ---init ---recursive' to make sure the submodules are at the correct heads.
*   Make sure you have the correct AUDIO_BOARD_PATH selected for your board.
*   Delete the build/ directory and also sdkconfig and sdkconfig.old and then build and flash again.
*   If you are still facing issues, reproduce the issue on the default example for the device and then contact Espressif for help. Make sure to share these:
    *   The steps you followed to reproduce the issue.
    *   Complete device logs (from device boot-up) taken over UART.
    *   <voice_assistant>.elf file from the build/ directory.
    *   If you have gdb enabled, run the command 'backtrace' and share the output of gdb too.
    *   The esp-va-sdk and esp-idf branch you are using and the AUDIO_BOARD_PATH that you have set.

## A1.4 Device not crashed but not responding

My device is not responding to audio queries:

*   Make sure your device is connected to the Wi-Fi/Internet.
*   If the device is not taking the wake-word, make sure the mic is turned on.
*   Try using the Tap-To-Talk button and then ask the query.
*   If you are still facing issues, reproduce the issue on the default example for the device and then contact Espressif for help. Make sure to share these:
    *   The steps you followed to reproduce the issue.
    *   Complete device logs taken over UART.
    *   <voice_assistant>.elf file from the build/ directory.
    *   The esp-va-sdk and esp-idf branch you are using and the AUDIO_BOARD_PATH that you have set.

**Also check the Appendix sections in the respective voice assistant's directories.**
