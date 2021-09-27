idf_component_register(SRCS "${ESP_CODEC_PATH}/esp_codec.c" "${ESP_CODEC_PATH}/media_hal_codec_init.c"
                    INCLUDE_DIRS "include"
                    PRIV_INCLUDE_DIRS "${ESP_CODEC_PATH}"
                    REQUIRES media_hal audio_hal)
