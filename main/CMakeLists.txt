file(GLOB_RECURSE SOURCES "*.cpp")

idf_component_register(SRCS ${SOURCES}
                    PRIV_REQUIRES 
                        spiffs
                        mqtt
                        nvs_flash
                        esp_wifi
                        nvs_flash
                    REQUIRES
                        azure-iot-middleware-freertos
                        arduinoFFT
                        SensorLib
                        ArduinoJson
                    INCLUDE_DIRS ".")
spiffs_create_partition_image(storage ../certs/myiotdevice1 FLASH_IN_PROJECT)