file(GLOB_RECURSE SOURCES "*.cpp")

idf_component_register(SRCS ${SOURCES}
                    PRIV_REQUIRES spiffs
                    PRIV_REQUIRES mqtt
                    PRIV_REQUIRES nvs_flash
                    PRIV_REQUIRES esp_wifi
                    PRIV_REQUIRES nvs_flash
                    INCLUDE_DIRS ".")
spiffs_create_partition_image(storage ../certs/myiotdevice1 FLASH_IN_PROJECT)