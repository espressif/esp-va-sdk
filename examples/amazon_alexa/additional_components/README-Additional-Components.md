## Enabling Equalizer [Optional]
Equalizer lets you control the Bass, Mid-Range and Treble of the audio. You can use the following commands to get the values for the equalizer:
* Set Treble to 3
* Set Bass to -3
* Reset Equalizer
* Set Movie mode        // check this
The SDK will give a callback to the application (in equalizer.c) with the respective values for the equalizer. The application can then use these values and adjust the audio output.

To enable the equalizer along with Alexa follow these steps:
* Enable Equalizer from menuconfig
```
make menuconfig -> Voice Assistant Configurations -> Enable Equalizer
```
* In the application (app_main.c), include `equalizer.h`.
* Call `equalizer_init()` before calling `alexa_init()`.

## Enabling Local Player [Optional]
Local Player lets you play local audio files stored on the device. Take a look at local_player.c to get a basic idea on how to use this.

To enable the local player along with Alexa follow these steps:
* Enable Local Player from menuconfig
```
make menuconfig -> Voice Assistant Configurations -> Enable Local Player
```
* In the application (app_main.c), include `local_player.h`.
* Call `local_player_init()` after calling `alexa_init()`.

## Enabling AWS IoT [Optional]
To Enable AWS IoT platform on ESP32 follow below steps:

* Enable Amazon Web Services IoT Platform
```
make menuconfig -> Component config -> Amazon Web Services IoT Platform
```
* Enter AWS IoT Endpoint Hostname
```
make menuconfig -> Component config -> Amazon Web Services IoT Platform -> AWS IoT Endpoint Hostname -> Enter the hostname
```

* Copy certificate file (certificate.pem.cert and private.pem.key) to additional_components/va_aws_iot/certs/ folder appropriately to communicate with AWS endpoint.

* Subscribe to "test_topic/esp32" on AWS Console to see the messages.

### Locally Check The Root Certificate [Optional]
The Root CA certificate provides a root-of-trust when the ESP32 connects to AWS IoT. We have supplied the root CA certificate already (in PEM format) in the file `additional_components/va_aws_iot/certs/aws-root-ca.pem`.

If connection to server fails, you might want to locally verify that this Root CA certificate hasn't changed, you can run the following command against your AWS MQTT Host:

```
openssl s_client -showcerts -connect hostname:8883 < /dev/null
```

(Replace hostname with your AWS MQTT endpoint host.) The Root CA certificate is the last certificate in the list of certificates printed. You can copy-paste this in place of the existing `aws-root-ca.pem` file.

### Demo
* App supports usual demo scenario for Alexa as described in README-Alexa.md.
* When AWS-IoT is enabled, each time "Alexa" wakeword is detected the WakeWord count is incremented on AWS IoT console.