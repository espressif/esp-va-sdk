## ChangeLog

### v1.0b1r6 (Beta) - 2018-12-24

**Enhancements**

* Support for local mp3 playback. Example application has also been added.
* Support for EqualizerController interface. Just facilitating APIs are added, user should register and implement callback function for actual Equalizer control.
* Support for CLI-based PlaybackController interface. Play, pause, next, previous, button and toggle commands are supported. "button" CLI can be used to emulate button press on the device. Or application can call appropriate API from playback_controller.h to notify SDK of button press (if user wants to implement hardware button).
* Support for CLI based WiFi provisioning. App is still required for Amazon authentication.

**API Changes**

* Removed support for static authentication (AVS partition based authentication).

**Bug Fixes**

* Fixed bugs related to stability and device getting stuck in infinite loop in some cases.

**Known Issues/Improvements**

* Only limited TuneIn radio stations are supported
* It is largely tested with internet and WiFi connectivity intact throughout its operation. Some issues are seen when device loses connectivity.

### v1.0b1r5 (Beta) - 2018-11-12

**Enhancements**

* TLS Certificate validation of Amazon and other streaming sites.

**API Changes**

* http_connection_new now requires pointer to esp_tls_cfg_t object.
* Check application's README for a new patch on esp-idf, without which compilation will fail with this release.

**Bug Fixes**

* Device goes in infinite loop if internet goes down while responding to "What's up?"
* Other minor exception fixes

**Known Issues/Improvements**

* Only limited TuneIn radio stations are supported
* It is largely tested with internet and WiFi connectivity intact throughout its operation. Some issues are seen when device loses connectivity.

### v1.0b1r4 (Beta) - 2018-09-28

**Enhancements**

* Support for PLS playlist parser
* Support for Pandora music streaming service
* TuneIn stations using PLS playlist format are now supported
* LyraTD-MSC board support

**API Changes**

* Older http_stream.c/.h is now renamed to http_playback_stream.c/.h. All APIs starting with http_stream are renamed to http_playback_stream
* http_stream.c/.h file now contains APIs which are useful to create a stream to POST data to cloud and receive response on the same connection and stream

**Bug Fixes**

* Stability related fixes
* Fixed crashes seen in Audible when segments >512 are received

**Known Issues/Improvements**

* Only limited TuneIn radio stations are supported
* It is largely tested with internet and WiFi connectivity intact throughout its operation. Some issues are seen when device loses connectivity.
* TLS Certificate validation of Amazon and other streaming sites is yet to be done.

### v1.0b1r3 (Beta) - 2018-08-28

**Enhancements**

* Seeking support for Audible books
* Support for OffsetInMilliSeconds, ProgressReportDelayElapsed and ProgressReportIntervalElapsed
* LED patterns for various Alexa states as per Alexa UX design guidelines
* Playing Alerts/Timers, Reminders from received URL even after device reboot. Device used to play stored tunes if device rebooted in between setting the Alarm and its timeout
* Minute latency improvement where Recognize event is now being sent earlier upon WW detection

**Bug Fixes**

* No intermittent audio playback between multiple Speak directives of the same dialog (e.g "What's up")
* Few stability related fixes

**Known Issues**

* Crashes are seen in Audible when segments >512 are received
* Only limited TuneIn radio stations are supported
* It is largely tested with internet and WiFi connectivity intact throughout its operation. Some issues are seen when device loses connectivity.

### v1.0b1r2 (Beta) - 2018-08-14

**Enhancements**

* Support for persistent Alerts/Timers, Reminders and Notifications

**Bug Fixes**

* Multiple race conditions fixed to improve the stability

**Known Issues**

* Regression in Audible support - crashes are seen with Audible
* Book and audio playback seeking is not yet supported
* Only limited TuneIn radio stations are supported
* It is largely tested with internet and WiFi connectivity intact throughout its operation. Some issues are seen when device loses connectivity.

### v1.0b1r1 (Beta) - 2018-08-06

**Enhancements**

* Amazon music support
* Alerts/Timers, Reminders, Notifications support
* Audible support
* Capabilities API integration
* AudioActivityTracker support

**Bug Fixes**
* Overall stability improvements
* Fixed minor issues with Android App
* Amazon app is no more required on phone for authentication. App opens login page on device's browser if app isn't found.

**Known Issues**

* Alerts/Timers, Reminders and Notifications do not persist across device reboot
* Book and audio playback seeking is not yet supported
* Only limited TuneIn radio stations are supported
* It is largely tested with internet and WiFi connectivity intact throughout its operation. Some issues are seen when device loses connectivity.

### v1.0a1r1 (Alpha) - 2018-07-09

* Complete C-Based SDK from ground-up with support for
    * Basic conversation
    * Multi-turn
    * Audio playback - Saavn, TuneIn, Kindle
    * Volume control (both physical and spoken)
    * Tap-To-Talk
* Salvaged 75KB of internal memory as compared to CPP based port
* Phone app (Android) support for:
    * Network configuration
    * Alexa authentication

### v0.15

* Port of Amazon's CPP SDK (v1.3) with additional optimizations for embedded target
