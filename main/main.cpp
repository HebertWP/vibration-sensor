#include <Arduino.h> 
#include "DEV_config.h"

#include "ble_setup.h"
#include "QMI8658_setup.h"
#include "iot_setup.h"

extern "C" void app_main(void)
{
    esp_err_t err;
    initArduino();
    Serial.begin(115200);
    err = setupFiles();
    if (err != ESP_OK)
    {
        ESP_LOGE("MAIN", "Failed to setup files");
        ESP_LOGE("MAIN", "Error code: %s", esp_err_to_name(err));
        ESP_LOGE("MAIN", "Restarting...");
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        esp_restart();
    }
    setupBLE();
    setupQMI8658();
    setupIOT();
}