set(COMPONENT_ADD_INCLUDEDIRS ./)

# Edit following two lines to set component requirements (see docs)
set(COMPONENT_REQUIRES json_parser voice_assistant esp_adc_cal)
set(COMPONENT_PRIV_REQUIRES media_hal console audio_hal nvs_flash audio_utils wifi_provisioning led_pattern led_driver button_driver)

set(COMPONENT_SRCS ./json_utils.c ./str_utils.c ./strdup.c ./va_button.c ./va_diag_cli.c ./va_led.c ./va_mem_utils.c ./va_nvs_utils.c ./va_file_utils.c ./wifi_cli.c ./va_time_utils.c ./network_diagnostics.c)

register_component()
