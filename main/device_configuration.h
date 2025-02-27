#ifndef _DEVICE_CONFIG_H_
#define _DEVICE_CONFIG_H_

#include "esp_wifi.h"

#include "Arduino.h"
#include <ArduinoJson.h>

/**
 * GPIOI config
 **/

#define LCD_DC_PIN (8)
#define LCD_CS_PIN (9)
#define LCD_CLK_PIN (10)
#define LCD_MOSI_PIN (11)
#define LCD_MISO_PIN (12)
#define LCD_RST_PIN (14)
#define LCD_BL_PIN (2)

#define DEV_SDA_PIN (6)
#define DEV_SCL_PIN (7)
#define DEV_INT2_PIN (3)

#define Touch_INT_PIN (5)
#define Touch_RST_PIN (13)

#define BAT_ADC_PIN (1)
// #define BAR_CHANNEL     (A3)


/**
 * Sensors CONFIG
 **/

#define SENSOR_I2C_ADDR (0x68)
#define SENSOR_I2C_FREQ (400000)

/**
 * BLE UUIDs
 **/
static const uint16_t SSID_UUID = 0xFF01;
static const uint16_t PASSWORD_UUID = 0xFF02;


/*
 * FILES PATHS
 */
const char device_config_json_path[] = "/spiffs/device_config.json";

/* 
 * GLOBAL VARIABLES
*/


extern wifi_sta_config_t g_wifi_connection;
extern DynamicJsonDocument *g_device_document;

#endif
