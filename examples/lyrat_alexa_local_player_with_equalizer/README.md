# Prerequisites
Please prepare your host system with the toolchain. Please see http://esp-idf.readthedocs.io/en/latest/get-started/index.html for the host setup.

# Supported Hardware
The current application supports ESP32 based LyraT v4.1, LyraT v4.2 and LyraT v4.3

# Prepare Images
If you already have the pre-compiled images flashed on the board, please jump to the next section (Device Configuration).

## Clone all the repositories
```
$ git clone --recursive https://github.com/espressif/esp-idf.git

$ cd esp-idf; git checkout release/v3.1; cd ..

$ git clone https://github.com/espressif/esp-avs-sdk.git
```

## Applying extraneous patches on esp-idf
```
$ cd esp-idf

$ git apply ../esp-avs-sdk/esp-idf-patches/memset-i2s-dma-buffers-zero.patch

$ git apply ../esp-avs-sdk/esp-idf-patches/esp-tls-Add-support-for-global-CA-store.-All-mbedtls.patch
```

## Build and flash the project
```
$ cd esp-avs-sdk/examples/test/lyrat_alexa_local_player_with_equalizer

$ export IDF_PATH=/path/to/esp-idf

$ export ESPPORT=/dev/cu.SLAB_USBtoUART (or /dev/ttyUSB0 or /dev/ttyUSB1 on Linux or COMxx on MinGW)

$ make -j 8 flash monitor
```

# Device Configuration
Android and iOS apps are available to configure WiFi credentials and associate user's Amazon account with the device.
* [Android](https://github.com/espressif/esp-avs-sdk/releases)
* [iOS](https://github.com/espressif/esp-idf-provisioning-ios/tree/versions/avs)

# Demo
* Once the board boots up and successfully connects to the Wi-Fi network after provisioning, device will initialize Alexa and will start playing local audio from SD card. Below is the sequence of local playback:
  * Play 1.mp3 for 10 seconds
  * Play 2.mp3 (This will interrupt 1.mp3 and start playing 2.mp3)
  * Enqueue 3.mp3 and 1.mp3 (This will be played after 2.mp3 completes playback)
* You can connect any external speaker/headphone with 3.5mm connector to PHONE JACK to listen to playback/Alexa responses.
* Local playback can be interrupted to talk to Alexa. You can press and release Tap-to-talk button and speak. The green LED glows when the microphone is active.
* you can now ask any command like:
    * tell me a joke
    * how is the weather?
    * will it rain today?
    * Sing a song
    * Play TuneIn radio
    * Set volume level to 7
* After the response is completely played, device will resume local playback.
* Press and Hold "Mode" button for 3 seconds to reset the board to factory settings
