#include "DEV_config.h"
#include "stdio.h"

#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <ArduinoJson.h>
#include "esp_spiffs.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "nvs_handle.hpp"
#include "freertos/FreeRTOS.h"
#include "esp_system.h"

#define DEVICE_CONFIG_FILE "/src/DEV_config.cpp"

DynamicJsonDocument *deviceDocument = new DynamicJsonDocument(1024);
char ssid[128];
char password[128];

esp_err_t initSPIFFS()
{
    esp_err_t ret;
    // Initialize SPIFFS
    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/spiffs",
        .partition_label = NULL,
        .max_files = 5,
        .format_if_mount_failed = true};

    ret = esp_vfs_spiffs_register(&conf);

    switch (ret)
    {
        case ESP_OK:
        {
            ESP_LOGI(DEVICE_CONFIG_FILE, "Successfully initialized SPIFFS");
            break;
        }
        case ESP_FAIL:
        {
            ESP_LOGE(DEVICE_CONFIG_FILE, "Failed to mount or format filesystem");
        }
        case ESP_ERR_NOT_FOUND:
        {
            ESP_LOGE(DEVICE_CONFIG_FILE, "Failed to find SPIFFS partition");
        }
        default:
        {
            ESP_LOGE(DEVICE_CONFIG_FILE, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
        }
    }
    return ret;
}

esp_err_t init_nvs()
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_LOGI(DEVICE_CONFIG_FILE, "NVS partition was truncated and needs to be erased");
        err = nvs_flash_erase();
        if (err != ESP_OK)
        {
            ESP_LOGE(DEVICE_CONFIG_FILE, "Failed to erase NVS partition");
        }else
        {
            err = nvs_flash_init();
            if (err != ESP_OK)
            {
                ESP_LOGE(DEVICE_CONFIG_FILE, "Failed to initialize NVS partition");
            }
        }
    }
    return err;
}

esp_err_t internal_write_nvs(nvs_handle handle, const char *key, char *value)
{
    esp_err_t err = nvs_set_str(handle, key, value);
    if (err != ESP_OK)
    {
        ESP_LOGE(DEVICE_CONFIG_FILE, "Failed to set value in NVS");
        return err;
    }
    err = nvs_commit(handle);
    if (err != ESP_OK)
    {
        ESP_LOGE(DEVICE_CONFIG_FILE, "Failed to commit value in NVS");
    }
    return err;
}

esp_err_t read_nvs(nvs_handle handle, char *array, const char *key)
{
    size_t len;
    esp_err_t err = nvs_get_str(handle, key, NULL, &len);
    if (err != ESP_OK)
    {
        ESP_LOGE(DEVICE_CONFIG_FILE, "Failed to get value from NVS (%s)", key);
        ESP_LOGI(DEVICE_CONFIG_FILE, "Setting default value in NVS");
        array[0] = '\0';
        return internal_write_nvs(handle, key, array);
    }
    ESP_LOGI(DEVICE_CONFIG_FILE, "Read %s from NVS", key);
    err = nvs_get_str(handle, key, array, &len);
    if (err != ESP_OK)
    {
        ESP_LOGE(DEVICE_CONFIG_FILE, "Error (%s) reading!\n", esp_err_to_name(err));
    }
    return err;
}

esp_err_t get_handle(nvs_handle *handle)
{
    esp_err_t err = nvs_open("nvs", NVS_READWRITE, handle);
    if (err != ESP_OK)
    {
        ESP_LOGE(DEVICE_CONFIG_FILE, "Error (%s) opening NVS handle!\n", esp_err_to_name(err));
    }
    return err;
}

esp_err_t write_nvs(const char *key, char *value)
{
    nvs_handle handle;
    esp_err_t err = get_handle(&handle);
    if (err != ESP_OK)
    {
        ESP_LOGE(DEVICE_CONFIG_FILE, "Failed to get NVS handle");
        return err;
    }
    err = internal_write_nvs(handle, key, value);
    if (err != ESP_OK)
    {
        ESP_LOGE(DEVICE_CONFIG_FILE, "Failed to write value to NVS");
        return err;
    }
    nvs_close(handle);
    return ESP_OK;
}

esp_err_t setupFiles()
{
    esp_err_t err = initSPIFFS();
    if (err != ESP_OK)
    {
        ESP_LOGE(DEVICE_CONFIG_FILE, "Failed to initialize SPIFFS");
        return err;
    }
    err = init_nvs();
    if (err != ESP_OK)
    {
        ESP_LOGE(DEVICE_CONFIG_FILE, "Failed to initialize NVS");
        return err;
    }

    FILE *f = fopen("/spiffs/device_config.json", "r");
    char line[128];

    fgets(line, sizeof(line), f);
    deserializeJson(*deviceDocument, line);
    fclose(f);

    nvs_handle handle;
    err = get_handle(&handle);
    if (err != ESP_OK)
    {
        ESP_LOGE(DEVICE_CONFIG_FILE, "Failed to get NVS handle");
        return err;
    }

    err = read_nvs(handle, ssid, SSID_UUID);
    if (err != ESP_OK)
    {
        ESP_LOGE(DEVICE_CONFIG_FILE, "Failed to read SSID from NVS");
        return err;
    }
    err = read_nvs(handle, password, PASSWORD_UUID);
    if (err != ESP_OK)
    {
        ESP_LOGE(DEVICE_CONFIG_FILE, "Failed to read password from NVS");
        return err;
    }
    nvs_close(handle);
    return ESP_OK;
}