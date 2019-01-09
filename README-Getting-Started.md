# Prerequisites
Please prepare your host system with the toolchain. Please see http://esp-idf.readthedocs.io/en/latest/get-started/index.html for the host setup.

# Supported Hardware
lyrat and lyrat_sr application supports ESP32 based LyraT v4.1, LyraT v4.2 and LyraT v4.3
lyratd_msc_sr application supports ESP32 based LyraTD_MSC v2.0, LyraTD_MSC v2.1

# Prepare Images

## Clone all the repositories
```
$ git clone --recursive https://github.com/espressif/esp-idf.git

$ cd esp-idf; git checkout release/v3.1; cd ..

$ git clone https://github.com/espressif/esp-avs-sdk.git
```

## Apply patches on esp-idf
```
$ cd esp-idf

$ git apply ../esp-avs-sdk/esp-idf-patches/memset-i2s-dma-buffers-zero.patch

$ git apply ../esp-avs-sdk/esp-idf-patches/esp-tls-Add-support-for-global-CA-store.-All-mbedtls.patch
```

## Build and flash the project
```
$ cd esp-avs-sdk/examples/<example_board_directory>

$ export IDF_PATH=/path/to/esp-idf

$ export ESPPORT=/dev/cu.SLAB_USBtoUART (or /dev/ttyUSB0 or /dev/ttyUSB1 on Linux or COMxx on MinGW)

$ make -j 8 flash VOICE_ASSISTANT=<alexa/gva/dialogflow> monitor
```
NOTE:
> lyrat app only supports Tap-to-talk whereas lyrat_sr and lyratd_msc_sr apps support both, "Alexa" wakeword and tap-to-talk.
