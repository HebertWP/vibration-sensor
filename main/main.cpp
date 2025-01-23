#include <Arduino.h>
#include <esp_spiffs.h> 

#include "ble_setup.h"
#include "QMI8658_setup.h"
#include "iot_setup.h"

extern "C" void app_main(void)
{
    esp_err_t ret;
    //Initialize SPIFFS, it's used to save the the device configuration and certificates
    esp_vfs_spiffs_conf_t conf = {
      .base_path = "/spiffs",
      .partition_label = NULL,
      .max_files = 5,
      .format_if_mount_failed = true
    };

    ret = esp_vfs_spiffs_register(&conf);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE("app_main", "Failed to mount or format filesystem");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            ESP_LOGE("app_main", "Failed to find SPIFFS partition");
        } else {
            ESP_LOGE("app_main", "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
        }
        return;
    }

    //using arduino functions, to import Arduino libraries
    initArduino();
    Serial.begin(115200);
    
    setupBLE();
    setupQMI8658();
    setupIOT();
}