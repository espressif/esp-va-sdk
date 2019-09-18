# ESP_HTTPD_OTA

The application can call esp_httpd_ota_update_init() with a callback function and an existing server (optional).

A URI handler will be registered for OTA: <ip_addr>:80/update

On a local network, the user can do a post request on the above URI with the new binary file.

As soon as the post request is made, a callback will be given to the application to do any pre-OTA changes. Then the downloading and storing of the update in a separate partition will start in chunks of 2KB. Once that is complete, another callback will be given to the application to do any post-OTA changes.

After that, the device will be restarted with the new binary.
