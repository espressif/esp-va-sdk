# Table Of Contents

* [1. Project Setup](#1-project-setup)
    * [1.1 Enabling Bluetooth](#11-enabling-bluetooth)
        * [1.1.1 Building firmware](#111-building-firmware)
        * [1.1.2 Using Bluetooth](#112-using-bluetooth)
* [2. Device Performance](#2-device-performance)
    * [2.1 CPU and Memory usage](#21-cpu-and-memory-usage)
* [3. Test Case Specific Instructions](#3-test-case-specific-instructions)
    * [3.1 Wi-Fi AP Change (Test Case 1.7)](#31-wi-fi-ap-change-test-case-17)
    * [3.2 Device Reset (Test Case 1.9)](#32-device-reset-test-case-19)
    * [3.3 Sign Out (Test Case 1.6)](#33-sign-out-test-case-16)


# 1. Project Setup

## 1.1 Enabling Bluetooth

Optionally, bluetooth music streaming can be enabled on the device.

### 1.1.1 Building firmware

Remove the older build for sdkconfig.bt.defaults to set the config again
```
rm -rf build/ sdkconfig sdkconfig.old
```
Now export the bluetooth variable before compiling again
```
export ALEXA_BT=1
```

### 1.1.2 Using Bluetooth

You can connect to the BT device with either of the following methods:
*   Go to the BT settings of your phone and connect to ESPXXXX device, which is discoverable.
*   Ask, "Alexa, connect my phone". This will either connect to a previously paired device, or Alexa will ask to pair a new device from your phone's BT settings.
*   In the `Amazon Alexa` app, discover and connect to a BT device which has been paired before.

Once connected you can stream music from your phone.

**Note:** Currently only A2DP_SINK mode is operational. i.e, you can stream music from other devices to ESP, not vice versa.

# 2. Device Performance

## 2.1 CPU and Memory usage

The following is the CPU and Memory Usage.

|                                       |Up And Running |Normal Queries |Music          |
|:-                                     |:-:            |:-:            |:-:            |
|**CPU Usage**                          |-              |-              |-              |
|**Free Internal Memory (328KB DRAM)**  |115KB          |111KB          |103KB          |
|**Free External Memory (4MB PSRAM)**   |1.10MB         |1.08MB         |0.99MB         |

**Flash Usage**: Firmware binary size: 3.9MB

This should give you a good idea about the amount of CPU and free memory that is available for you to run your application's code. For the free memory, a minimum of 50KB of free internal memory is required for healthy operation.


# 3. Test Case Specific Instructions

These instructions are with reference to the document *Amazon AVS Functional Qualification Tests v3.2*.

## 3.1 Wi-Fi AP Change (Test Case 1.7)

This test requires us to change the Wi-Fi configuration of the device. Please refer to the Section [Button Functions](#131-buttons), for details about how the Wi-Fi configuration can be erased.

Once the Wi-Fi configuration is erased, the deivce will be back in the configuration mode. You can relaunch the phone app, and begin the configuration process as described in the Section [Configuration](#41-configuration-steps). In this case, the phone app will detect that the Alexa configuration is already done and skip those options.

Please note that this Wi-Fi only device configuration mode stays enabled only for 3 minutes. After 3 minutes, the device will reboot and connect to the already configured Wi-Fi Access Point.

## 3.2 Device Reset (Test Case 1.9)

This test requires us to reset the device to its factory state. Please refer to the Section [Button Functions](#131-buttons), for details about how the device can be reset to factory settings.

Once the device is reset, the device will be back in the configuration mode. You can relaunch the phone app, and begin the configuration process as described in the Section [Configuration](#41-configuration-steps).

## 3.3 Sign Out (Test Case 1.6)

This test requires us to sign-out of Alexa. This can be done through the companion phone app.

Ensure that the phone is within the same Wi-Fi network as the device. Launch the phone app and go to *Manage Devices*. A list of devices will be visible to you. Click on the device, and then click on Sign-out.

You can sign back in again, using the companion app using the same workflow. Ensure that the phone is within the same Wi-Fi network as the device. Launch the phone app and go to *Manage Devices*. A list of devices will be visible to you. Click on the device, and then click on Sign-in. Follow the sign-in steps to sign-in to the device.
