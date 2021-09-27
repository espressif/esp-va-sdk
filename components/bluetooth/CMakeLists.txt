
if(CONFIG_BT_ENABLED)
    if(NOT(CONFIG_BTDM_CONTROLLER_MODE_BLE_ONLY))

        set(srcs "src/bluetooth.c" "src/bt_app_av.c")
        set(priv_include_dirs src)
        set(include_dirs ./include/)

    endif()
endif()

idf_component_register(SRCS "${srcs}"
                       INCLUDE_DIRS "${include_dirs}"
                       PRIV_INCLUDE_DIRS "${priv_include_dirs}"
                       REQUIRES bt audio_utils uuid-gen sys_playback)
