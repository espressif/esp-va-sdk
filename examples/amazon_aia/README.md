# Table Of Contents

* [1. Project Setup](#1-project-setup)
    * [1.1 AWS IoT Configuration](#11-aws-iot-configuration)
        * [1.1.1 Using Espressif's AWS Account](#111-using-espressif's-aws-account)
        * [1.1.2 Using Your AWS Account](#112-using-your-aws-account)
    * [1.2 Reusing the AWS IoT MQTT connection](#12-reusing-the-aws-iot-mqtt-connection)
        * [1.2.1 Working with MQTT](#121-working-with-mqtt)
        * [1.2.2 Subscribing](#122-subscribing)
        * [1.2.3 Publishing](#123-publishing)
        * [1.2.4 Testing](#124-testing)
* [2. Device Performance](#2-device-performance)
    * [2.1 CPU and Memory usage](#21-cpu-and-memory-usage)
* [3. Test Case Specific Instructions](#3-test-case-specific-instructions)
    * [3.1 Wi-Fi AP Change (Test Case 1.7)](#31-wi-fi-ap-change-test-case-17)
    * [3.2 Device Reset (Test Case 1.9)](#32-device-reset-test-case-19)
    * [3.3 Sign Out (Test Case 1.6)](#33-sign-out-test-case-16)
* [A2 Appendix FAQs](#a2-appendix-faqs)
    * [A2.1 Migrating to your own AWS account](#a21-migrating-to-your-own-aws-account)


# 1. Project Setup

## 1.1 AWS IoT Configuration

The Alexa SDK requires AWS IoT certificates to be configured in your firmware. If this is not done, even if the device boots-up, it would not yet successfully communicate with Alexa.

There are two ways to do this (a) You can either use temporary certificates from Espressif's AWS-IoT account for evaluation, or (b) You can use your own AWS account for generating the certificates. 

### 1.1.1 Using Espressif's AWS Account

If you wish to do a quick evaluation of AVS for AWS IoT and you do not have an AWS account, you can try it with temporary certificates created in Espressif's AWS account. Please follow this:

1. Certificate Generation:
    1. You can generate temporary AWS IoT certificates from here: [Generate Certificate](https://espressif.github.io/esp-va-sdk/).
    2. You will get an email with the device certificates. (Check your spam too.)
2. Certificate Flashing:
    1. After receiving the device certificates, modify the file mfg_config.csv and add the relevant paths for all the files.
    2. Now run the following command to generate the manufacturing partition (mfg.bin) for your device.
    ```
    python $IDF_PATH/components/nvs_flash/nvs_partition_generator/nvs_partition_gen.py generate /path/to/mfg_config.csv mfg.bin 0x6000
    ```
    3. Use the following command to flash the certificates on the device.
    ```
    $ python $IDF_PATH/components/esptool_py/esptool/esptool.py --chip esp32 --port $ESPPORT write_flash 0x10000 mfg.bin
    ```
    4. Flash the Alexa firmware again as described in the Section [Flashing the Firmware](#24-flashing-the-firmware) and reboot the device.
3. The device is now functional and you can skip to the [Device Provisioning](#4-device-provisioning) Section.

### 1.1.2 Using Your AWS Account

If you want to use your own AWS account for the AWS IoT certificates and using your account for all the Alexa communication, please follow these instructions for getting it in place.

1.  AWS Account Whitelisting:
    1.  Create an AWS account, if you don't have one: [AWS Account](https://aws.amazon.com/account/).
        1.  Once complete, note the AWS Account ID: [AWS Account Details](https://console.aws.amazon.com/billing/home?#/account)
    2.  Create your AVS product.
        1.  If you wish to do a quick evaluation you may skip the AVS product creation step, and use Espressif's AVS product instead. For a quick evaluation, this step will save you the hassle of regenerating the Android/iOS phone applications that should be linked with your AVS product. If you wish to try this, please contact your Espressif contact person for the procedure, and skip the next bullet item. If not, please proceed with the next steps.
        2.  Steps for creating the AVS product:
            1.  Go to [Amazon Developer Account](http://developer.amazon.com) and create an account. Then go to [AVS Product Page](https://developer.amazon.com/alexa/console/avs/home) and create an AVS product with 'Product category' as 'Smart Home'.
            2.  While doing so, make sure to select 'Yes' in the section 'Is this device associated with one or more AWS IoT Core Accounts?'. Enter your AWS Account ID (that you noted above) there and complete the creation of the product.
            3.  Note: You will also have to build your own Android and iOS app with this product_id and security_profile to sign-in into the device.
    3.  Additionally, your AWS account also needs to be configured with the appropriate CloudFormation templates. You only need to do this step if your AWS Account was created before the year 2020. If your AWS account was created later, feel free to ignore this setup.
        1.  Please get in touch with your Espressif contact for the CloudFormation templates. Please ensure that this configuration is also done.
    4.  You need to create a policy to be attached to the thing you will be creating in the next step.
        1.  Go to AWS Services -> IoT Core (AWS IoT) -> Secure -> Policies
        2.  Click on create.
        3.  Enter the Name: ESP-Alexa-Policy.
        4.  Under Add statements, click on 'Advanced mode'.
        5.  Replace the existing policy with this:
            ```
            {
              "Version": "2012-10-17",
              "Statement": [
                {
                  "Action": [
                    "iot:Publish",
                    "iot:Receive"
                  ],
                  "Resource": [
                    "arn:aws:iot:*:*:topic/$aws/*/${iot:Connection.Thing.ThingName}/*",
                    "arn:aws:iot:*:*:topic/$aws/things/${iot:Connection.Thing.ThingName}/shadow/*",
                    "arn:aws:iot:*:*:topic/node/${iot:Connection.Thing.ThingName}/*",
                    "arn:aws:iot:*:*:topic/${iot:Connection.Thing.ThingName}/*"
                  ],
                  "Effect": "Allow"
                },
                {
                  "Action": [
                    "iot:Connect"
                  ],
                  "Resource": [
                    "arn:aws:iot:*:*:client/${iot:Connection.Thing.ThingName}"
                  ],
                  "Effect": "Allow"
                },
                {
                  "Action": [
                    "iot:Subscribe"
                  ],
                  "Resource": [
                    "arn:aws:iot:*:*:topicfilter/$aws/*/${iot:Connection.Thing.ThingName}/*",
                    "arn:aws:iot:*:*:topicfilter/$aws/things/${iot:Connection.Thing.ThingName}/shadow/*",
                    "arn:aws:iot:*:*:topicfilter/node/${iot:Connection.Thing.ThingName}/*",
                    "arn:aws:iot:*:*:topicfilter/${iot:Connection.Thing.ThingName}/*"
                  ],
                  "Effect": "Allow"
                },
                {
                  "Action": [
                    "iot:GetThingShadow",
                    "iot:UpdateThingShadow"
                  ],
                  "Resource": [
                    "*"
                  ],
                  "Effect": "Allow"
                }
              ]
            }
            ```
        6.  Click on create.

2.  Generating AWS IoT Certificates:
    1.  Register a thing in AWS IoT as described in this tutorial: [How to register a thing](https://docs.aws.amazon.com/iot/latest/developerguide/register-device.html)
    2.  Please make sure that the thing is 'Activated'
    3.  Download 3 files for the thing:
        1.  Device Certificate
        2.  Device Private Key
        3.  Root CA for AWS IoT
    4.  You will need to attach the policy created above as created above. After clicking on 'Attach a policy', select ESP-Alexa-Policy from the policy list.
3.  Device Firmware: Configure AWS Settings:
    1.  Make a note of your AWS Account ID
        1.  This can be accessed from this location: [AWS Account Details](https://console.aws.amazon.com/billing/home?#/account)
    2.  Make a note of your AWS IoT Endpoint URL
        1.  This can be accessed from this location: AWS Services -> IoT Core (AWS IoT) -> Settings
    3.  Go to the example application *examples/amazon_aia/* and use
        the command:
        ```
        $ idf.py menuconfig
        ```
    4.  Go to 'Voice Assistant Configuration'.
    5.  Update the AWS Account ID to have your account id
    6.  Update the AWS Endpoint to have your IoT endpoint
4.  Device Firmware: Configure AWS IoT Certificates:
    1.  Copy the 3 files downloaded after creating the AWS IoT Thing above, to the directory *examples/amazon_aia/certs/*
    2.  Modify the file *examples/amazon_aia/certs/mfg_config.csv* and add the relevant paths for all the files.
    3.  Generate the manufacturing partition (mfg.bin) with the command:
        ```
        $ cd examples/amazon_alexa/certs

        $ python $IDF_PATH/components/nvs_flash/nvs_partition_generator/nvs_partition_gen.py generate mfg_config.csv mfg.bin 0x6000
        ```
    4.  Flash this manufacturing partition with the command:
        ```
        $ python $IDF_PATH/components/esptool_py/esptool/esptool.py --chip esp32 --port $ESPPORT write_flash 0x10000 mfg.bin
        ```
    5.  Flash the firmware again as described in the Section [Flashing the Firmware](#24-flashing-the-firmware) and reboot the device.

Now the device is functional.


## 1.2 Reusing the AWS IoT MQTT connection

The Alexa SDK uses MQTT to communicate with AWS IoT. You can use this same connection to publish or subscribe to your custom topics too. This reusing of the network connection for both voice and your device control allows you to have a more compact memory utilisation.

Example 1: You can subscribe to a topic which tells the device to turn on or turn off the lights, if you are building a lightbulb.

Example 2: You can implement OTA through publish and subscribe.

### 1.2.1 Working with MQTT

You can refer to *esp-va-sdk/examples/amazon_aia/main/app_mqtt.c*. It has the skeletal code for publishing and subscribing to custom topics using the same MQTT connection which is being used by the Alexa SDK.

The MQTT agent requires that the publish/subscribe interactions with AWS IoT happens in the same thread's context. This skeletal code allows you to do this with minimum hassle.

### 1.2.2 Subscribing

To begin with, you can subscribe to the topics of your interest in the *app_mqtt_subscribe_handler()* function. This function need to be called only once during initial boot-up as subscription to topics only has to happen once.

You can use the va_mqtt function, *va_mqtt_subscribe()*, to subscribe to your topics of interest. 

*app_mqtt_subscribe_callback()* is called whenever there is some data on the topic. If the *topic* argument in the callback is NULL, then the data is a part of the previous message and can be combined here.

### 1.2.3 Publishing

Any updates that you wish to publish on MQTT can be handled in the *app_mqtt_publish_handler()* function. You can call this function whenever you wish to publish to the topic.

You can use the va_mqtt function, *va_mqtt_publish()*, to publish to your desired topic.

### 1.2.4 Testing

You can use mosquitto, which is a MQTT broker, to test your custom publish and subscribe implementations.

You will need to install mosquitto on your Host machine. Then you can communicate with the device using the following commands:

Publish command:
```
$ mosquitto_pub --cafile server.crt --cert device.crt --key device.key -h <endpoint> -p 8883 -q 1 -t <mqtt_topic> -i <device_id> --tls-version tlsv1.2 -m '<your_payload>' -d
```
Subscribe command:
```
$ mosquitto_sub --cafile server.crt --cert device.crt --key device.key -h <endpoint> -p 8883 -q 1 -t <mqtt_topic> -i <device_id> --tls-version tlsv1.2 -d
```
In the commands above:

*   Add the path to the appropriate server.crt, device.crt and device.key files. You will need to create new certificates to be used or it may affect the device's AWS IoT MQTT connection.
*   Replace '<endpoint\>' with your endpoint.
*   Replace '<mqtt_topic\>' with the topic you want to publish/subscribe to.
*   Replace '<device_id\>' with a unique ID for your Host.
*   Replace '<your_payload\>' with the message you want to publish.


# 2. Device Performance

## 2.1 CPU and Memory usage

The following is the CPU and Memory Usage.

|                                       |Up And Running |Normal Queries |Music          |
|:-                                     |:-:            |:-:            |:-:            |
|**CPU Usage**                          |-              |-              |-              |
|**Free Internal Memory (328KB DRAM)**  |115KB          |115KB          |115KB          |
|**Free External Memory (4MB PSRAM)**   |2.48MB         |2.46MB         |2.46MB         |

**Flash Usage**: Firmware binary size: 3.4MB

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


# A2 Appendix FAQs

## A2.1 Migrating to your own AWS account

Where does it ask to link the product ID with the AWS Account?

*   While creating a new product, you will be asked 'Is this device associated with one or more AWS IoT Core Accounts?'. When you select 'Yes', you will be asked to enter the AWS Account ID 'Please provide your AWS Account ID(s) (comma separated)?'.

How to change the phone app for the new product ID?

*   For the documentation on Android and iOS apps, you can refer: [Android](https://github.com/espressif/esp-idf-provisioning-android/tree/versions/avs-ble) and [iOS](https://github.com/espressif/esp-idf-provisioning-ios/tree/versions/avs-ble).

My device is not connecting to AWS.

*   Make sure the device is activated. You can check this here: Go to IoT Core -> Manage -> Select your device -> Security -> Click on the certificate -> Actions -> Activate.
*   Make sure the correct policy is attached with the certificate. You can check this here: Go to IoT Core -> Manage -> Select your device -> Security -> Click on the certificate -> Policies -> Check the policy.

While booting, the device shows '{"code": "INVALID_AWS_ACCOUNT", "description": "The AWS IoT account provided does not support AIS"}' and keeps on booting.

*   Make sure the 'Registration Request Buffer' has the correct parameters.
*   The authentication token has been generated with the new product ID.
*   The device id i.e. the client ID is unique.
*   The AWS Account ID and Endpoint are correct.
*   If all of these are correct, try doing factory reset and try to setup the device again.
