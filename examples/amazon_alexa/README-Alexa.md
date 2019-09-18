# Introduction
Alexa is Amazon's personal virtual assistant which listens to user's voice commands and responds with appropriate answers. Apart from conversing with the user, Alexa lets you play music from a variety of music streaming services. Alexa also helps you manage to-do lists and allows for voice-assisted shopping from Amazon.

# Device Configuration
* Before proceeding with device configuration, make sure you have read and followed the [Getting Started Guide](../../README-Getting-Started.md). This will let you download and build the firmware, and then flash it on the board. Connect the board to your development host and power it on.
* By default, when the firmware comes up it is un-provisioned. The device console should display the following lines in this mode:
```
I (xxx) conn_mgr_prov: Provisioning started with :
	service name = ESP-Alexa-xxxx
	service key =
	proof of possession (pop): abcd1234
```
You can use either of the Android or iOS apps (links below) to provision your device to the desired Wi-Fi Access Point and associate the user's Amazon account with the device.
* [Android](https://play.google.com/store/apps/details?id=com.espressif.provbleavs)
* [iOS](https://apps.apple.com/in/app/esp-alexa/id1464127534)

# Demo
* Once the board boots up and successfully connects to the Wi-Fi network after provisioning, you will see a print "Alexa is ready", after which you can use either use "Rec" button on the board or say "Alexa" to start a conversation. For Tap-to-Talk, press and release the button and speak. The green LED glows when the microphone is active.
* You can connect any external speaker/headphone with 3.5mm connector to PHONE JACK to listen to responses.
* you can now ask any command like:
    * Tell me a joke
    * How is the weather?
    * Will it rain today?
    * Sing a song
    * Play TuneIn radio
    * Set volume to 7
* Press and Hold "Volume+" and "Volume-" button for 10 seconds to reset the board to factory settings.
* Press and Hold "Action" button for 5 seconds to just reset the wifi settings.

# Supported audio streaming services
* Amazon Music
* JioSaavn (India)
* Gaana (India)
* Hungama (India)
* myTuner (India)
* Audible
* Kindle
* Pandora
* TuneIn Radio
* iHeart Radio
* TedTalks

# Production notes
* In order to create Alexa-enabled commercial products, Amazon certified acoustic front-end has to be used. Please reach out to Espressif if you are looking to go to production.
