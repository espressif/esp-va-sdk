idf_component_register(SRCS "app_wifi.c" "app_prov.c"
                    INCLUDE_DIRS "."
                    REQUIRES misc wifi_provisioning console qrcode)
