cmake_minimum_required(VERSION 3.16)

file(GLOB_RECURSE SOURCES "*.c*")
set(INCLUDE "." "./includes")

idf_component_register(
                        SRCS 
                            ${SOURCES}
                        REQUIRES 
                            spiffs
                            mqtt
                            nvs_flash
                            esp_wifi
                            nvs_flash
                            azure-iot-middleware-freertos
                            sample-azure-iot
                            arduinoFFT
                            SensorLib
                            ArduinoJson
                        INCLUDE_DIRS 
                            ${INCLUDE}
)

spiffs_create_partition_image(storage ../certs/myiotdevice1 FLASH_IN_PROJECT)