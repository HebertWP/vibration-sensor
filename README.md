# Vibration Sensor Project

## Overview

This project is designed to monitor vibrations using a sensor and communicate the data via MQTT. The project is built using the ESP32-S3 on ESP-IDF framework.

This project is create to be a base code to implement all the base functionalities of IOT sensor device.

## Features

- ✅ Add Arduino as component: for easy integration of Arduino's 
libraries.
- ✅ Load MQTT credential from SPIFFS: for easy configuration of MQTT device in production environment and remove of credentials from code.
- ✅ Have BLE GATT server: To allow users configure network configuration in real time.
- ✅ RF Coexistence: To have BLE and WiFi working together.
- ✅ Send Telemetry data to Azure IoT Hub.
- ✅ Receive Direct message from Azure IoT Hub.
- ✅ Receive Twin update from Azure IoT Hub.
- ❌ HTTPS OTA update.
- ✅ Acellerometer read.
- ✅ FFT processing for sensor data.
- ❌ Encrypted data in flash: To store sensitive data in flash not in plain text.
- ❌ Backup data when no network: To store data in flash when no network is available.
- ❌ Implement LVGL: To have on side display of data.


## Hardware

To facilitate the development of this project, I'm using a board from wave share:

```https://www.waveshare.com/wiki/ESP32-S3-Touch-LCD-1.28```

Note: This board is not on sales anymore, but you can use any ESP32-S3 board. End wave share have a even better board for this project, that have RTC and lager screen:

```https://www.waveshare.com/wiki/ESP32-S3-Touch-LCD-2.1```

## Software

To develop this project, I'm using the following software:

- Visual Studio Code: As IDE.
- ESP-IDF: As framework.
- Arduino: As component to use Arduino libraries.
- Azure SDK for C Arduino: To send data to Azure IoT Hub.
- ArduinoFFT: To process FFT data.
- SensorLib: To read data from sensor.
- ArduinoJson: To parse JSON data.

Note: The project use modules from git, so clone it using recursive.

## Code notes

The code use RF Coexistence, i have pinned the BLE stack in code 1 and WiFi stack in code 0. I also deactivate the software control to use hardware control of RF Coexistence, for easy code. For more information about RF Coexistence, please check the following link:

```https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-guides/coexist.html#rf-coexistence```

The code use Arduino as component, so this project stars from Arduino as component template. For more information:

```https://espressif-docs.readthedocs-hosted.com/projects/arduino-esp32/en/latest/esp-idf_component.html```

The code use SPIFFS to store MQTT credentials, on development i add the code on the build to copy my credentials to SPIFFS. For more information:

```https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/storage/spiffs.html```