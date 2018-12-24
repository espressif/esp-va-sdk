set(COMPONENT_ADD_INCLUDEDIRS include)
set(COMPONENT_PRIV_INCLUDEDIRS src)
set(COMPONENT_SRCS  "src/wifi_prov.c"
                    "src/wifi_prov_handlers.c"
                    "src/wifi_prov_mode_ble.c"
                    "src/wifi_prov_mode_softap.c")

set(COMPONENT_REQUIRES protocomm)
set(COMPONENT_PRIV_REQUIRES wifi_provisioning)

register_component()
