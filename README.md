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

The SDK can easily be extended to other ESP32 based audio platforms that have SPIRAM availability.

## Getting started

* When flashing the SDK for the first time, it is recommended to do `make erase_flash` to wipe out entire flash and start out fresh.
* Please refer to [LyraT README](examples/lyrat_alexa/README.md) to get started with flashing, provisioning and Alexa interactions.
