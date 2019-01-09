# Enabling AWS [Optional]
To Enable AWS IoT platform on ESP32 follow below steps:

* Enable Amazon Web Services IoT Platform
```
make menuconfig -> Component config -> Amazon Web Services IoT Platform
```
* Enter AWS IoT Endpoint Hostname
```
make menuconfig -> Component config -> Amazon Web Services IoT Platform -> AWS IoT Endpoint Hostname -> Enter the hostname
```

* Copy certificate file (certificate.pem.cert and private.pem.key) to examples/lyrat_alexa_sr/main/certs/ folder appropriately to communicate with AWS endpoint.

* Subscribe to "test_topic/esp32" on AWS Console to see the messages.

### Locally Check The Root Certificate [Optional]
The Root CA certificate provides a root-of-trust when the ESP32 connects to AWS IoT. We have supplied the root CA certificate already (in PEM format) in the file `main/certs/aws-root-ca.pem`.

If connection to server fails, you might want to locally verify that this Root CA certificate hasn't changed, you can run the following command against your AWS MQTT Host:

```
openssl s_client -showcerts -connect hostname:8883 < /dev/null
```

(Replace hostname with your AWS MQTT endpoint host.) The Root CA certificate is the last certificate in the list of certificates printed. You can copy-paste this in place of the existing `aws-root-ca.pem` file.

# Demo
* App supports usual demo scenario for each voice assistant as described in the respective README in the SDK root directory.
* When AWS-IoT is enabled, each time Alexa wakeword is detected the WakeWord count is incremented on AWS IoT console.
* Board only support "Alexa" wakeword as of now. Hence even to use Google Voice Assistant or Dialogflow one must say "Alexa" to wake the device.
