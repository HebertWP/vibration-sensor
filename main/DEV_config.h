#ifndef _DEV_CONFIG_H_
#define _DEV_CONFIG_H_


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

/* BLE UUIDs
*/

static const char* SSID_UUID = "0xFF01";
static const uint16_t SSID_UUID16 = 0xFF01;

static const char* PASSWORD_UUID = "0xFF02";
static const uint16_t PASSWORD_UUID16 = 0xFF02;

extern char ssid[128];
extern char password[128];

extern DynamicJsonDocument *deviceDocument;

esp_err_t setupFiles();
esp_err_t write_nvs(const char *key, char *value);
#endif
