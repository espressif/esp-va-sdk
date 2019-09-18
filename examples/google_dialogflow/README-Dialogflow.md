# Introduction 
Dialogflow (previously known as API.AI) is a voice enabled conversational interface from Google.
It enables IoT users to include natural language user interface in their applications, services and devices.

The advantages of Dialogflow wrt voice assistants are less complexity, pay as you go pricing, custom wakeword allowed and no certification hassles.

Unlike voice-assistants, Dialogflow let's you configure every step of the conversation, and it won't answer other trivia/questions like voice-assistants typically do. For e.g. A Dialogflow agent for Laundry project will provide information only about the configurable parameters of the laundry (like state, temperature, wash cycle etc.)

This release facilitates the audio communication of ESP32 with a Google Dialogflow agent using its v2beta1 gRPC APIs

# Dialogflow Agent Setup
You will have to create a Dialogflow account and setup a Dialogflow agent in the cloud. This agent configuration is where you will specify what conversations will you be supporting.
* Follow this [link](https://dialogflow.com/docs/getting-started) to get started.
* Create your own agent
    * You can add intents, entities, actions and parameters as per your agent's requirements.
    * Build, test your agent and validate the responses using the console on Dialogflow.
* Optionally, you can add an existing sample agent from [here](https://dialogflow.com/docs/samples) to your Dialogflow account and use the same.
Note:
> Make sure that "Set this intent as end of conversation" is enabled under "Responses" tab in each intent of your Dialogflow agent so that the device can use this information to close the interaction with user

# Device Configuration
* Your device needs to be configured with the correct credentials to talk to the above project.
* Navigate to this [link](https://console.cloud.google.com/apis/dashboard)
    * Select the agent created above as the project
    * Go to Credentials section (on the left)
        * Credentials -> Create credentials -> OAuth client ID -> Other
        * OAuth consent screen -> Fill in the Support email (and other details as required) -> Save
        * Credentials -> OAuth 2.0 client IDs -> Download the created OAuth client ID file (`client_secret_<client-id>.json`)
* Follow the steps specified in this [link](https://developers.google.com/assistant/sdk/guides/library/python/embed/install-sample#generate_credentials)
    * While using this step, use the following command instead of the one specified:
```
google-oauthlib-tool --scope https://www.googleapis.com/auth/cloud-platform \
    --save --headless --client-secrets /path/to/client_secret_<client-id>.json
```

* Modify the example application (app_main.c) provided in this SDK, to add the project ID (from `client_secret_<client-id>.json` file) in the `project_name` member of `device_config` before making a call to `dialogflow_init()`
* Once you download credentials.json, you can use the following commands on the device console to set the client ID, client secret and refresh token on the device.
* Make sure to enter the nvs-set commands first and then the wifi-set command.
```
[Enter]
>> nvs-set refreshToken string <refresh_token_from_credentials.json>
>> nvs-set clientId string <client_id_from_credentials.json>
>> nvs-set clientSecret string <client_secret_from_credentials.json>
```
* Use below CLI command to configure device's station interface
```
[Enter]
>> wifi-set <ssid> <passphrase>
```

# Demo
* Once the board successfully connects to the Wi-Fi network, you can use the "Rec" button on the board to start a conversation. (Support for wakeword will be available soon.) For Tap-to-Talk, press and release the button and speak.
* For an example Laundry project, one can set below configurable parameters while creating the project as described above:
    * State: On/Start or Off/Stop
    * Temperature: Valid temperature values
    * Wash Cycle: Heavy, Medium or Light
* Now you can wake the device by pressing "Rec" button and say command like:
    * Start the laundry with temperature 68 and heavy wash cycle
* You can also initiate multi-turn conversations like:
    * Turn on the laundry
    * (Dialogflow: At what temperature) 75
    * (Dialogflow: What is the wash cycle) Light wash cycle
* The Assistant's language can be changed by setting an appropriate code string va_cfg->device_config.device_language in app_main.c. List of valid code strings can be found [here](https://dialogflow.com/docs/reference/language).

NOTE:
> Once a multi-turn conversation is in-progress you do not need to press "Rec" button for every turn.
