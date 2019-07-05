## Overview

The ESP-Voice-Assistant SDK provides an implementation of Amazon's Alexa Voice Service, Google Voice Assistant and Google's conversational interface (aka Dialogflow) for ESP32 microcontroller. This facilitates the developers to evaluate ESP32 based voice assistant/s integrated devices like speakers and other IoT devices. Please refer to [Changelog](CHANGELOG.md) to track release changes and known-issues.

## License
* For LyratD-DSPG board based on DSPG's DBMD5P DSP please read the licensing terms [here](board_support_pkgs/lyratd_dspg/dspg_fw/docs/license.pdf) for the DSP fimrware
* For rest of the ESP-VA-SDK components please refer to the licensing terms [here](LICENSE)

### About SDK

The SDK contains pre-built libraries for Amazon Alexa, Google Voice Assistant (GVA) and Google Dialogflow along with sources of some of the utility components such as audio pipeline and connection manager. Below are the list of some of the features supported for each voice assistant:
* **Amazon Alexa**:
    * Basic conversation
    * Multi-turn conversations
    * Audio Streaming and Playback
    * Audio Book Support: Kindle, Audible
    * Volume control via voice command
    * Seek support for Audible
    * Alarms, Timers, Reminders, Notifications

* **Google Voice Assistant**:
    * Basic conversation
    * Multi-turn conversations
    * Getting weather reports
    * Multiple language support

* **Google Dialogflow**:
    * Basic conversation
    * Multi-turn conversations
    * Configure and control connected devices via voice, e.g "Start the Laundry"
    * Multiple language support

## Supported Hardware

The SDK supports the following hardware platforms:
* [ESP32-LyraT](https://www.espressif.com/en/products/hardware/esp32-lyrat)
* [ESP32-LyraTD-MSC](https://www.espressif.com/en/products/hardware/esp32-lyratd-msc)

The following list of acoustic front-ends is also supported. Please contact Espressif to enable acccess to these solutions.
* DSPG DBMD5P
* Intel s1000
* Synaptics CX20921

## Getting started

* Follow the `README-Getting-Started.md` to clone the required repositories and to compile and flash the firmware.
* When flashing the SDK for the first time, it is recommended to do `make erase_flash` to wipe out entire flash and start out fresh.
* Go through `README-<voice_assistant>.md` in their respective directories to know how to provision the device and to get authentication tokens from the respective authorization server and flash them onto the device.
* If you are updating from previous release, please check the `Upgrading from Previous Release` section from `README-Getting-Started.md` to know about any specific actions that needs to be taken while upgrading.
