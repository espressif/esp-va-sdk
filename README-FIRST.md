## Upgrading to v1.0b1r7 (Beta)

* Most API names have changed from alexa\_ to va\_. Please follow these steps to upgrade.
* Download script v1_0r6Tov1_0r7_update_script.sh available under "releases" tab, and copy it in SDK's root directory (esp-voice-assistants).
* Copy your example in examples directory.
* Run the script using below commands:
    * cd /path/to/esp-voice-assistants
    * ./v1_0r6Tov1_0r7_update_script.sh

## Upgrading to v1.0b1r6 (Beta)

* Release v1.0b1r6 changes the way WiFi credentials are stored in NVS. Hence when you upgrade your device to latest SDK, your device would go into provisioning mode again, and needs to be re-provisioned via provisioning app.
