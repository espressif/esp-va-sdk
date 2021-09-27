# Table Of Contents

* [1. Audio Hal](#1-audio-hal)
    * [1.1 File Structure](#11-file-structure)
* [2. Customising your board](#2-customising-your-board)
    * [2.1 Getting Started](#21-getting-started)
    * [2.2 Button driver](#22-button-driver)
        * [2.2.1 Using a different button driver](#221-using-a-different-button-driver)
        * [2.2.2 Writing your own button driver](#222-writing-your-own-button-driver)
    * [2.3 LED Pattern](#23-led-pattern)
        * [2.3.1 Using a different LED pattern](#231-using-a-different-led-pattern)
        * [2.3.2 Writing your own LED pattern](#232-writing-your-own-led-pattern)
    * [2.4 LED Driver](#24-led-driver)
        * [2.4.1 Using a different LED driver](#241-using-a-different-led-driver)
        * [2.4.2 Writing your own LED driver](#242-writing-your-own-led-driver)
    * [2.5 Audio codec driver (esp_codec)](#25-audio-codec-driver-esp_codec)
        * [2.5.1 Using a different audio codec driver](#251-using-a-different-audio-codec-driver)
        * [2.5.2 Writing your own audio codec driver](#252-writing-your-own-audio-codec-driver)
    * [2.6 DSP Driver](#26-dsp-driver)
        * [2.6.1 Using a different DSP driver](#261-using-a-different-dsp-driver)
        * [2.6.2 Writing your own DSP driver](#262-writing-your-own-dsp-driver)
    * [2.7 Adding a new peripheral component](#27-adding-a-new-peripheral-component)
* [3. Changes from previous versions](#3-changes-from-previous-versions)
    * [3.1 File changes](#31-file-changes)
    * [3.2 API changes](#32-api-changes)


# 1. Audio Hal

## 1.1 File Structure

```
audio_hal
    |- audio_board
    |- button_driver
    |- led_pattern
    |- led_driver
    |- esp_codec
    |- dsp_driver
```
```
audio_hal                           : base directory
    |- CMakeLists.txt               : for audio_hal: cmake
    |- component.mk                 : for audio_hal: make
    |- media_hal.c                  : common file for esp_codec
    |- va_dsp.c                     : common file for dsp_driver
    |- include                      : includes for audio_hal

|- audio_board                      : directory for all audio boards
    |- audio_board_board_name       : directory for a specific audio board
        |- audio_board.cmake        : points to all the drivers needed for the board: cmake
        |- audio_board.mk           : points to all the drivers needed for the board: make
        |- audio_board              : audio board specific files
            |- va_board.c           : high level initialisations and configurations of the peripheral components
            |- audio_board.c        : pin-config and the initialisations of the peripherals like i2s, i2c
            |- CMakeLists.txt       : for audio_board: cmake
            |- component.mk         : for audio_board: make

|- button_driver                    : directory for button driver
    |- CMakeLists.txt               : common for all button drivers: cmake
    |- component.mk                 : common for all button drivers: make
    |- include                      : includes for button_driver
        |- button_driver.h          : common include
    |- button_driver_name           : specific button driver
        |- button_driver.c          : initialisation and event handling for the button_driver

|- led_pattern                      : directory for led pattern
    |- CMakeLists.txt               : common for all led patterns: cmake
    |- component.mk                 : common for all led patterns: make
    |- include                      : includes for led_pattern
        |- led_pattern.h            : common include
    |- led_pattern_arrangement      : directory for all types of led patterns for a particular arrangement
        |- led_pattern_type         : pattern for a particular type
            |- led_pattern.c        : initialisation and patterns for the led_pattern

|- led_driver                       : directory for led driver
    |- CMakeLists.txt               : common for all led drivers: cmake
    |- component.mk                 : common for all led drivers: make
    |- include                      : includes for led_driver
        |- led_driver.h             : common include
    |- led_driver_name              : specific led driver
        |- led_driver.c             : initialisation and setting state for led_driver

|- esp_codec                        : directory for audio codec driver (esp_codec)
    |- CMakeLists.txt               : common for all codec drivers: cmake
    |- component.mk                 : common for all codec drivers: make
    |- esp_codec_name               : specific codec driver
        |- esp_codec.h              : APIs for the driver
        |- esp_codec.c              : implementation of APIs for the driver
        |- media_hal_codec_init.c   : middle layer between media_hal and esp_codec

|- dsp_driver                       : directory for dsp driver
    |- common_dsp                   : common driver APIs
        |- CMakeLists.txt           : for common_dsp: cmake
        |- component.mk             : for common_dsp: make
        |- common_dsp.h             : common APIs
        |- common_dsp.c             : implementation common APIs which can be used by the dsp driver
    |- dsp_driver_name              : specific dsp driver
        |- CMakeLists.txt           : for specific dsp driver: cmake
        |- component.mk             : for specific dsp driver: make
        |- va_dsp_hal.c             : middle layer between va_dsp and dsp_driver with implementations of APIs for the driver
        |- firmware.bin             : wake word engine firmware binary
```

# 2. Customising your board

## 2.1 Getting Started

The SDK is so architected that it is quite easy to rebuild the SDK for your board with a minimal set of changes to the core.

From where to start:
*   If your board is similar to one of the existing boards, then it is easiest to copy-paste that audio_board_\<board_name\> directory and start customising from there.
*   If your board is very different from any of the existing boards, then it is best to start with the reference audio_board_hollow which is available within the SDK at *components/audio_hal/audio_board/audio_board_hollow*
*   Point the *AUDIO_BOARD_PATH* to this new board so that it is selected and used by the SDK.

Basic structure:
*   The *va_board.c* file contains all the high level initialisations and configurations of the peripheral components like LED, audio codec, DSP driver, etc.
*   The *audio_board.c* file contains the pin-config and the initialisations of the peripherals like i2s, i2c, etc. These initialisations are done by the peripheral components as they need.
*   The *audio_board.cmake* and *audio_board.mk* files specify and include the peripheral components of the board like buttons, LEDs, audio codec, DSP driver, etc.

How to proceed:
*   The customisation details of the various peripheral components is listed below.
*   Along with customising peripheral components, make sure that the pin config and the initialisations of the peripherals, corresponsing to the peripheral components, is also correct.
*   Each peripheral component can be selected one at a time and changed or written from scratch with the help of 'hollow' versions of them.
*   Make sure the *audio_board.cmake* and *audio_board.mk* files correctly point to the required peripheral component.

<hr>

## 2.2 Button driver

### 2.2.1 Using a different button driver

Out of the box, the SDK supports the button driver for buttons connected through GPIO or through ADC using a resistor divider circuit. You can switch the button driver by changing the *BUTTON_DRIVER_PATH* appropriately in your *audio_board.cmake* and *audio_board.mk* files.

The selected button driver will be initialised in *va_board_button_init()* by calling the *button_driver_init()* API for that driver, which plugs it into the SDK's button_driver module (*va_button_init()* initializes the SDK's button handling module). The button driver configurations for button events can be done in *va_board_button_init()*.

#### 2.2.1.1 ADC

Button events for various button presses (these can be single button or a combination of multiple buttons) are initialised in the *button_driver_config_t* structure by specifying the voltage values for those button presses. If a button event does not exist, a '-1' entry is used to indicate this to the SDK. A tolerance value is also configured. 

For finding the ADC values:
*   Theoretical: Calculate the voltage (in mV) across ADC pin and GND when the button is pressed, using the voltage divider method. This voltage (in mV) is the value for that button or button combination.
*   Experimental: *button_driver_enable_debug(true)* can be called to print the ADC values and easily find the experimental values for the buttons and button combinations.

#### 2.2.1.2 GPIO

Button events for various button presses (these can be single button or a combination of multiple buttons) are initialised in the *button_driver_config_t* structure by specifying the GPIO values for those button presses. If a button event does not exist, a '-1' entry is used to indicate this to the SDK.

The GPIO numbers in the button event value is the bit numbers (in 64 bits). Example:
*   GPIO 30: ((int64_t)1) << 30
*   GPIO 30 and GPIO 36: ((int64_t)1) << 30 | ((int64_t)1) << 36

### 2.2.2 Writing your own button driver

If the Button driver that you wish to use is not part of Espressif's supported list, you can write a driver for it yourself.

A reference hollow_button is available within the SDK at *components/audio_hal/button_driver/hollow_button*. This includes all the skeletal code and the empty APIs that the button driver is supposed to implement to plug into the SDK.

The driver has to implement the APIs in *button_driver.c*. These typically include APIs for initializing the driver and checking for button events. Take a look at *button_driver.h* for API definitions. You can also take a look at other button drivers for reference.

The API *button_driver_get_event()* is called repeatedly from the SDK to check if any button event has been triggered. Typically, the button events which can be a combintaion of buttons, might need to be checked first. The actions on the button event are performed by the SDK according to the event returned by this API.

The configurations and the button event values that this driver needs can be done from *va_board_button_init()* in *va_board.c*

Once this driver is implemented, use this driver as mentioned in the subsection for [Using a different button driver](#21-using-a-different-button-driver).

<hr>

## 2.3 LED Pattern

### 2.3.1 Using a different LED pattern

Out of the box, the SDK supports the LED patterns for configurations: single, linear_5 and radial_12. The LED patterns for Alexa are imported from Alexa's standard resource package. You can switch the LED pattern by changing the *LED_PATTERN_PATH* appropriately in your *audio_board.cmake* and *audio_board.mk* files.

The selected pattern will be initialised in *va_board_led_init()* by calling the *led_pattern_init()* API for that pattern and then passed to *va_led_init()* which plugs into the SDK's LED handling module.

### 2.3.2 Writing your own LED pattern

Note: Please make sure your custom LED pattern will pass the voice assistant's certification before embarking on this task.

You can completely define your own LED pattern.

*   Each LED is of the format R-G-B represented as hex values 0xRRGGBB. (R-G-B is the default assumed. If your LED configuration is B-G-R or any other configuration, then it should be handled in the led_driver.) Some examples:
    *   `0xFF0000`: Red
    *   `0x880000`: Dimmer Red
    *   `0x00FF00`: Green
    *   `0x0000FF`: Blue
    *   `0x00FFFF`: Cyan (Green + Blue)
    *   `0xFFFFFF`: White
    *   `0x000000`: Black (LED off)
*   One LED state is a series of upto 12 (MAX_LEDS) RGB values, along with a delay in milliseconds representing the time for which the LED state should be visible. (led_pattern_state_t). Some examples:
	*   `{75, {0x0000FF}}`: State for 1 LED. The 1 LED will be blue for 75ms.
	*   `{100, {0x00FF00,0x00FF00,0x00FF00,0x00FF00,0x00FF00,0x00FF00,0x00FF00}}`: State for 7 LEDs. All 7 LEDs will be green for 100ms.
	*   `{50, {0xFF0000,0xFF0000,0xFF0000,0xFF0000,0xFF0000}}`: State for 5 LEDs. All 5 LEDs will be red for 50ms.
	*   `{50, {0xFF0000,0xFF0000,0x0000FF,0x0000FF,0x0000FF}}`: State for 5 LEDs. First 2 LEDs will be red and the other 3 LEDs will be blue for 50ms.
	*   `{50, {0x0000FF,0xFF0000,0x0000FF,0xFF0000,0x0000FF}}`: State for 5 LEDs. Second and fourth LEDs will be red and first, third and fifth LEDs will be blue for 50ms.
*   One event, say listening state, can have a number of LED states one after the other to create a pattern (led_pattern_config_t). Some examples:
    *   ```
        {50, {0x000022,0x000022,0x000022,0x000022,0x000022}},
        {50, {0x000055,0x000055,0x000055,0x000055,0x000055}},
        {50, {0x000088,0x000088,0x000088,0x000088,0x000088}},
        {50, {0x0000AA,0x0000AA,0x0000AA,0x0000AA,0x0000AA}},
        {50, {0x0000CC,0x0000CC,0x0000CC,0x0000CC,0x0000CC}},
        {50, {0x0000FF,0x0000FF,0x0000FF,0x0000FF,0x0000FF}},
        ```
        Pattern for 5 LEDs. All 5 LEDs start from very dim blue and go to bright blue.
    *   ```
        {90, {0x0000FF,0x000000,0x000000,0x000000,0x000000}},
        {90, {0x000000,0x0000FF,0x000000,0x000000,0x000000}},
        {75, {0x000000,0x000000,0x0000FF,0x000000,0x000000}},
        {75, {0x000000,0x000000,0x000000,0x0000FF,0x000000}},
        {60, {0x000000,0x000000,0x000000,0x000000,0x0000FF}},
        {60, {0x0000FF,0x000000,0x000000,0x000000,0x000000}},
        {45, {0x000000,0x0000FF,0x000000,0x000000,0x000000}},
        {45, {0x000000,0x000000,0x0000FF,0x000000,0x000000}},
        {30, {0x000000,0x000000,0x000000,0x0000FF,0x000000}},
        {30, {0x000000,0x000000,0x000000,0x000000,0x0000FF}},
        ```
        Pattern for 5 LEDs. First LED is blue and others are off at the start. Then the second is blue and others are off, then the third and so on. The speed of the pattern also increases.
*   Some led patterns might be looping. If the pattern is looping or not, is specific to the pattern and is mentioned in *led_pattern.h*. Some examples:
    *   ```
        {50, {0x0000FF,0x0000FF,0x0000FF,0x0000FF,0x0000FF}},
        {50, {0x000000,0x000000,0x000000,0x000000,0x000000}},
        ```
        Looping pattern for 5 LEDs. All 5 LEDs will be blue and will blink.
    *   ```
        {50, {0x0000FF,0x0000FF,0x0000FF,0x0000FF,0x0000FF}},
        {50, {0x000000,0x0000FF,0x0000FF,0x0000FF,0x000000}},
        {50, {0x000000,0x000000,0x0000FF,0x000000,0x000000}},
        {50, {0x000000,0x0000FF,0x0000FF,0x0000FF,0x000000}},
        ```
        Looping pattern for 5 LEDs. All 5 LEDs will be blue at the start, then the middle 3 LEDs will be blue and others off, then only the middle LED will be blue and others off, then middle 3 blue, then all 5 blue, then middle 3 blue and so on.
*   You have to define such patterns for all the events supported in the system, listening state, speaking state, alert state, etc.

The best way to get started is to look at *components/audio_hal/led_pattern/linear_5/alexa/led_pattern.c* as a reference that defines the pattern for 5 linear LEDs.

Once this pattern is defined, use this pattern as mentioned in the subsection for [Using a different LED pattern](#31-using-a-different-led-pattern).

<hr>

## 2.4 LED Driver

### 2.4.1 Using a different LED driver

Espressif has production-ready drivers for a known set of LED drivers that we support out of the box. Please reach out to your Espressif representative to get a list of these drivers. Once you have the driver, you can rebuild the SDK by modifying your *audio_board.cmake* and *audio_board.mk* files to point to the appropriate LED driver.

The selected LED driver will be initialised in *va_board_led_init()* by calling the *led_driver_init()* API for that driver, which plugs it into the SDK's led_driver module.

### 2.4.2 Writing your own LED driver

If the LED driver that you wish to use is not part of Espressif's supported list, you can write a driver for it yourself.

A reference hollow_led is available within the SDK at *components/audio_hal/led_driver/hollow_led*. This includes all the skeletal code and the empty APIs that the LED driver is supposed to implement to plug into the SDK.

The driver has to implement the APIs in *led_driver.c*. These typically include APIs for initializing the driver and controlling the LEDs. Take a look at *led_driver.h* for API definitions. You can also take a look at other LED drivers for reference.

The API *led_driver_set_value()* is called from the SDK with the led_value array. Each LED in the array is of the format R-G-B represented as hex values 0xRRGGBB. R-G-B is the default assumed. If your LED configuration is B-G-R or any other configuration, then it can be changed accordingly in this API.

If there are any configurations that this driver needs, that can be done from *va_board_led_init()* in *va_board.c*

Once this driver is implemented, use this driver as mentioned in the subsection for [Using a different LED driver](#41-using-a-different-led-driver).

<hr>

## 2.5 Audio codec driver (esp_codec)

### 2.5.1 Using a different audio codec driver

Espressif has production-ready drivers for a known set of audio codecs that we support out of the box. Please reach out to your Espressif representative to get a list of these drivers. Once you have the driver, you can rebuild the SDK by modifying your *audio_board.cmake* and *audio_board.mk* files to point to the appropriate audio codec driver.

The selected codec will be initialised in *va_board_init()* by calling the *media_hal_init()* API for that codec, which plugs it into the SDK's media_hal module.

### 2.5.2 Writing your own audio codec driver

If the audio codec that you wish to use is not part of Espressif's supported list, you can write a driver for it yourself.

A reference hollow_codec is available within the SDK at *components/audio_hal/esp_codec/hollow_codec*. This includes all the skeletal code and the empty APIs that the codec driver is supposed to implement to plug into the SDK.

The driver has to implement the APIs in *esp_codec.c*. These typically include APIs for controlling the volume setting mute/unmute, I2S and power up/down configurations. Take a look at *esp_codec.h* for API definitions. You can also take a look at other audio codecs for reference.

Once the APIs are implemented, they need to be assigned to the media_hal layer in *media_hal_codec_init.c* which will then be called by the SDK. The codec also needs to be configured and initialized here.

Once this driver is implemented, use this driver as mentioned in the subsection for [Using a different audio codec driver](#51-using-a-different-audio-codec-driver).

<hr>

## 2.6 DSP Driver

### 2.6.1 Using a different DSP driver

Espressif has production-ready drivers for a known set of DSP SoCs that we have partnered with. Please reach out to your Espressif representative to get a list of these drivers. Once you have the driver, you can rebuild the SDK by modifying your *audio_board.cmake* and *audio_board.mk* files to point to the appropriate DSP driver.

The selected DSP driver can be configured in *va_board_init()* by calling *va_dsp_hal_configure()* API and it will be initialised in *va_dsp_init()* by calling the *va_dsp_hal_init()* API for that driver.

### 2.6.2 Writing your own DSP driver

If the DSP driver that you wish to use is not part of Espressif's supported list, you can write a driver for it yourself.

A reference hollow_driver is available within the SDK at *components/audio_hal/dsp_driver/hollow_driver*. This includes all the skeletal code and the empty APIs that the DSP driver is supposed to implement to plug into the SDK.

The driver has to implement the APIs in *va_dsp_hal.c*. These typically include APIs for initializing the driver, microphone data processing and passing onto the SDK, etc. Take a look at *va_dsp_hal.h* for API definitions. You can also take a look at other DSP drivers for reference.

The driver might also need the model for wake-word detection. This can be added in the firmware.bin and then used in *va_dsp_hal.c* 

If there are any configurations that this driver needs, that can be done from *va_board_init()* in *va_board.c*

Once this driver is implemented, use this driver as mentioned in the subsection for [Using a different DSP driver](#61-using-a-different-dsp-driver).

<hr>

## 2.7 Adding a new peripheral component

The best way to add a new peripheral component so that it is maintained easily is to add another directory in *components/audio_hal/* for your component.

Add the new component in the *audio_board.cmake* and *audio_board.mk* files so that it is included when the audio_board is selected. Also make sure the *CMakeLists.txt* and *component.mk* files are present in the new component.


# 3. Changes from previous versions

## 3.1 File changes
```
audio_hal
    |- va_dsp.c                                             : common file

|- audio_board
    |- audio_board_board_name
        |- audio_board_board_name -> audio_board            : generic name
            |- audio_board_board_name.c -> audio_board.c    : generic name
            |- ab_button.c -> x                             : removed file

|- button_driver                                            : new driver

|- led_driver
    |- CMakeLists.txt                                       : common file
    |- component.mk                                         : common file
    |- include
        |- led_driver.h                                     : common file
    |- led_driver_name
        |- led_driver_name.c -> led_driver.c                : generic name

|- esp_codec
    |- CMakeLists.txt                                       : common file
    |- component.mk                                         : common file
    |- esp_codec_name
        |- esp_codec_name.c -> esp_codec.c                  : generic name
        |- esp_codec_name.h -> esp_codec.h                  : generic name

|- dsp_driver
    |- dsp_driver_name
        |- dsp_driver_name                                  : removed 1 directory level
        |- va_dsp -> x                                      : removed and made common
        |- va_dsp_hal.c                                     : new file

```

## 3.2 API changes

```
- audio_board.cmake, audio_board.mk: For peripheral components/drivers, DRIVER_PATH is set to the name of the driver (eg. LED_DRIVER_PATH is set to ws2812) and the component added is the directory of the path (eg. $(AUDIO_HAL_PATH)/led_driver)

audio_board:
Button:
- button_driver_init(&button_driver_config)                 : new API with generic name
        Param: button_driver_config_t                       : generic config
- va_button_init()                                          : Moved from va_board.c to app_main.c

LED:
- led_pattern_init(&ab_led_conf) -> led_pattern_init()      : params change
- led_driver_init(&led_driver_config)                       : new API with generic name
        Param: led_driver_config_t                          : generic config
- va_led_init()                                             : moved from va_board.c to app_main.c

Codec:
- media_hal_init_playback(&media_hal_playback_cfg)          : moved from va_board.c to media_hal.c
- media_hal_init(&media_hal_cfg) -> media_hal_init(&media_hal_cfg, &media_hal_playback_cfg)                 : params change

DSP:
- va_dsp_hal_configure((void *)&dsp_driver_config)          : new API with generic name
        Param: dsp_driver_config                            : config specific to the driver

Other:
- ab_button_gpio_init() -> audio_board_audio_jack_init()    : renamed and moved defination from ab_button.c to audio_board.c

button_driver:
- button_driver_get_event()                                 : new API

led_pattern:
- led_pattern_state_t.led_state_loop -> x                   : removed from the structure
- led_pattern_t                                             : renamed the patterns
- led_pattern_get_config(led_pattern_config_t **led_pattern_config)                                         : new API

led_driver:
- va_led_set_pwm(const uint32_t * led_value) -> led_driver_set_value(const uint32_t *led_value)             : rename

va_button:
- va_button_init(const button_cfg_t *button_cfg, int (*button_event_cb)(int)) -> va_button_init()           : params change
- Common button task for all the button drivers. Getting the button event from the button driver by calling the button_driver_get_event() API and taking the voice assistant specific action according to that.
- Moving the gpio/adc specific initialization to the respective button drivers.
- Moving the configuration and events to button driver.

va_led:
- va_led_init(led_pattern_config_t va_led_conf[LED_PATTERN_PATTERN_MAX]) -> va_led_init()                   : params change

voice_assistant:
- ais_mqtt_init(aia_config_t *cfg, void (*app_aws_iot_cb)(void)) -> aia_init(aia_config_t *cfg)             : params change and name change
- AIA now uses esp_mqtt instead of aws_iot for the mqtt client.
- ais_early_init() -> aia_early_init()                      : name change

va_dsp:
- va_dsp_init(speech_recognizer_recognize, speech_recognizer_record) -> va_dsp_init(speech_recognizer_recognize, speech_recognizer_record, va_button_notify_mute)       : params change
        Param: va_button_notify_mute()                      : callback if dsp has been muted already on bootup
- va_boot_dsp_signal()                                      : moved from va_dsp.c to app_main.c

```
