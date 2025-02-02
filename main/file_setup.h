#ifndef _FILE_SETUP_H_
#define _FILE_SETUP_H_

#include "esp_err.h"

/**
 * @brief Initialize memory, including NVS and SPIFFS.
 *
 * This function initializes the Non-Volatile Storage (NVS) and SPIFFS
 * file system. It should be called during the system initialization.
 * It's reads the data and store in global variables.
 *
 * @return
 *     - ESP_OK: Success
 *     - ESP_ERR_NO_MEM: Out of memory
 *     - ESP_ERR_NVS_NO_FREE_PAGES: NVS partition is full
 *     - ESP_ERR_NVS_NEW_VERSION_FOUND: NVS partition contains data in new format
 *     - ESP_ERR_NVS_NOT_INITIALIZED: NVS is not initialized
 */
esp_err_t init_memory();

/**
 * @brief Write a key-value pair to NVS.
 *
 * This function writes a key-value pair to the Non-Volatile Storage (NVS).
 * The value is stored as a string.
 *
 * @param[in] key The key to write.
 * @param[in] value The value to write.
 *
 * @return
 *     - ESP_OK: Success
 *     - ESP_ERR_NVS_NOT_FOUND: The key does not exist
 *     - ESP_ERR_NVS_INVALID_NAME: The key name is invalid
 *     - ESP_ERR_NVS_INVALID_LENGTH: The value length is invalid
 *     - ESP_ERR_NVS_NOT_ENOUGH_SPACE: There is not enough space to write the value
 */
esp_err_t write_nvs(const char *key, char *value, uint16_t len);

#endif