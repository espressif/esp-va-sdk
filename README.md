## Overview

The ESP-Alexa SDK provides an implementation of Amazon's Alexa Voice Service endpoint for ESP32 microcontroller. This facilitates the developers to evaluate ESP32 based Alexa integrated devices like speakers and IoT devices. Please refer to [Changelog](CHANGELOG.md) to track release changes and known-issues.

### About SDK

The SDK contains pre-built library of Alexa SDK along with sources of some of the utility components such as audio pipeline and connection manager. The SDK supports all major features of Alexa such as:
* Basic Alexa conversation
* Alexa dialogues and multi-turn
* Audio Streaming and Playback: Saavn, Amazon music, TuneIn (Only limited stations are supported as of now)
* Audio Book Support: Kindle, Audible
* Volume control via Alexa command
* Seek support for Audible
* Alerts/Timers, Reminders, Notifications

For now, Tap-To-Talk is the only interaction mode supported on LyraT.

## Supported Hardware

Release supports following hardware platforms:
* [ESP32-LyraT](https://www.espressif.com/en/products/hardware/esp32-lyrat)
* [ESP32-LyraTD-MSC](https://www.espressif.com/en/products/hardware/esp32-lyratd-msc)

The SDK can easily be extended to other ESP32 based audio platforms that have SPIRAM availability.

## Getting started

* SDK is designed to work with esp-idf 3.1 release. However, there are some changes needed in the IDF in order for the Alexa application to work. Please check esp-idf-patches directory for additional patches on top of esp-idf 3.1 release.
* When flashing the SDK for the first time, it is recommended to do `make erase_flash` to wipe out entire flash and start out fresh.
* Please refer to example READMEs to get started with flashing, provisioning and Alexa interactions.
