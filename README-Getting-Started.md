# Prerequisites
Please prepare your host system with the toolchain. Please see https://docs.espressif.com/projects/esp-idf/en/v3.2/get-started/index.html for the host setup.


# Prepare Images

## Clone all the repositories
```
$ git clone --recursive https://github.com/espressif/esp-idf.git

$ cd esp-idf; git checkout release/v3.2; git submodule update --init --recursive; cd ..

$ git clone https://github.com/espressif/esp-va-sdk.git
```

## Build and flash the project
```
$ cd esp-va-sdk/examples/<example_voice_assistant_directory>

$ export ESPPORT=/dev/cu.SLAB_USBtoUART (or /dev/ttyUSB0 or /dev/ttyUSB1 on Linux or COMxx on MinGW)

$ export IDF_PATH=/path/to/esp-idf

# Set audio_board path. e.g. For LyraT board:
$ export AUDIO_BOARD_PATH=/path/to/esp-va-sdk/board_support_pkgs/lyrat/audio_board/audio_board_lyrat/

$ make -j 8 flash monitor [ALEXA_BT=1]
```
* Once you have the firmware flashed, visit the following pages for interacting with the device:
   * [Alexa](examples/amazon_alexa/README-Alexa.md)
   * [Google Voice Assistant](examples/google_voice_assistant/README-GVA.md)
   * [DialogFlow](examples/google_dialogflow/README-Dialogflow.md)

## Enabling BT A2DP Sink support (Only for Alexa)
* In order to enable BT A2DP sink feature, please pass `ALEXA_BT=1` as command-line argument to make.
* Additional patches from idf_patches directory needs to be applied on ESP-IDF release/v3.2 branch using below commands:
    * git am </path/to/idf_patches/component-bt-implement-AVRCP-Target-APIs.patch>
    * git am </path/to/idf_patches/components-bt-Add-AVRCP-feature-about-volume.patch>

# Upgrading from Previous Release
Please skip this section if you are using the SDK for the first time.

## Upgrading to 1.2-RC1
* New firmware would require newer Android and iOS app for provisioning and local control. Please update apps from respective app stores.

## Upgrading to 1.0-RC2
* The partition table has been changed. If you face any issue, try doing 'make erase_flash' and then flash again.
* Authentication sequence has been changed for amazon_alexa. Refer to app_main.c in the amazon_alexa application.

## Upgrading to 1.0-RC1

* The example applications have changed from hardware specific to voice assistant specific.
  * Hardware/Board specific configuration (specifically, LEDs, Hardware Codecs, DSP and the board) is moved into a separate board support package. Please pass the path of this package to the build command line as indicated above.
* The esp-idf branch needs to be changed to release/v3.2 from release/v3.1 and the relevant new patches need to be applied.
* The nvs version is changed with IDF release/v3.2. Please do `make erase_flash` before flashing the new app. Now re-provision the device.
* Provisioning and authentication of Alexa app is now carried out over BLE instead of SoftAP. Use the new Android app for the same. (iOS app support is comming soon). GVA and Dialogflow would continue to use their respective older provisioning methods for now.


## Upgrading to v1.0b1r7 (Beta)

* Most API names have changed from alexa\_ to va\_. Please follow these steps to upgrade.
* Download script v1_0r6Tov1_0r7_update_script.sh available under "releases" tab, and copy it in SDK's root directory (esp-voice-assistants).
* Copy your example in examples directory.
* Run the script using below commands:
    * cd /path/to/esp-voice-assistants
    * ./v1_0r6Tov1_0r7_update_script.sh

## Upgrading to v1.0b1r6 (Beta)

* Release v1.0b1r6 changes the way WiFi credentials are stored in NVS. Hence when you upgrade your device to latest SDK, your device would go into provisioning mode again, and needs to be re-provisioned via provisioning app.
