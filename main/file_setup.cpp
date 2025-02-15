#include <esp_log.h>
#include "esp_err.h"

#include <nvs_flash.h>
#include "esp_spiffs.h"

#include "device_configuration.h"
#include "file_setup.h"

#define TAG "FILE_SETUP"

DynamicJsonDocument *g_device_document = new DynamicJsonDocument(1024);
wifi_sta_config_t g_wifi_connection = {};

esp_err_t init_SPIFFS()
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
        ESP_LOGI(TAG, "Successfully initialized SPIFFS");
        break;
    }
    case ESP_FAIL:
    {
        ESP_LOGE(TAG, "Failed to mount or format filesystem");
    }
    case ESP_ERR_NOT_FOUND:
    {
        ESP_LOGE(TAG, "Failed to find SPIFFS partition");
    }
    default:
    {
        ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
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
        ESP_LOGI(TAG, "NVS partition was truncated and needs to be erased");
        err = nvs_flash_erase();
        if (err != ESP_OK)
        {
            ESP_LOGE(TAG, "Failed to erase NVS partition");
        }
        else
        {
            err = nvs_flash_init();
            if (err != ESP_OK)
            {
                ESP_LOGE(TAG, "Failed to initialize NVS partition");
            }
        }
    }
    return err;
}

esp_err_t internal_write_nvs(nvs_handle handle, const char *key, char *value)
{
    ESP_ERROR_CHECK(nvs_set_str(handle, key, value));
    ESP_ERROR_CHECK(nvs_commit(handle));
    return ESP_OK;
}

esp_err_t read_nvs(nvs_handle handle, uint8_t *array, const char *key)
{
    size_t len;
    esp_err_t err = nvs_get_str(handle, key, NULL, &len);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to get value from NVS (%s)", key);
        ESP_LOGI(TAG, "Setting default value in NVS");
        array[0] = '\0';
        return internal_write_nvs(handle, key, (char *)array);
    }
    ESP_LOGI(TAG, "Read %s from NVS", key);
    ESP_ERROR_CHECK(nvs_get_str(handle, key, (char *)array, &len));
    return ESP_OK;
}

esp_err_t write_nvs(const char *key, char *value, uint16_t len)
{
    value[len] = '\0';
    nvs_handle handle;
    ESP_ERROR_CHECK(nvs_open("nvs", NVS_READWRITE, &handle));
    ESP_ERROR_CHECK(internal_write_nvs(handle, key, value));
    nvs_close(handle);
    return ESP_OK;
}

esp_err_t read_deviceconfig()
{
    FILE *f = fopen(device_config_json_path, "r");
    char line[128];

    fgets(line, sizeof(line), f);
    deserializeJson(*g_device_document, line);
    fclose(f);
    return ESP_OK;
}

esp_err_t init_memory()
{
    ESP_ERROR_CHECK(init_SPIFFS());
    ESP_ERROR_CHECK(init_nvs());
    ESP_ERROR_CHECK(read_deviceconfig());

    nvs_handle handle;
    ESP_ERROR_CHECK(nvs_open("nvs", NVS_READWRITE, &handle));

    char ssid_UUID[6];
    char password_UUID[6];
    sprintf(ssid_UUID, "%04X", SSID_UUID);
    sprintf(password_UUID, "%04X", PASSWORD_UUID);

    ESP_ERROR_CHECK(read_nvs(handle, g_wifi_connection.ssid, ssid_UUID));
    ESP_ERROR_CHECK(read_nvs(handle, g_wifi_connection.password, password_UUID));

    nvs_close(handle);
    return ESP_OK;
}