#include "esp_err.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"

#include "Arduino.h"

#include "file_setup.h"
#include "ble_setup.h"
#include "wifi_setup.h"
#include "QMI8658_setup.h"
#include "sample_azure_iot_pnp.c"

extern "C" void app_main(void)
{
    ESP_ERROR_CHECK(init_memory());
    //ESP_ERROR_CHECK(init_ble());
    ESP_ERROR_CHECK( esp_netif_init() );
    ESP_ERROR_CHECK( esp_event_loop_create_default() );
    /*Allow other core to finish initialization */
    vTaskDelay( pdMS_TO_TICKS( 100 ) );

    initArduino();
    Serial.begin(115200);
    setupQMI8658();
    ESP_ERROR_CHECK(init_wifi());
    vStartDemoTask();
}
//E (17370) esp-tls-mbedtls: mbedtls_ssl_setup returned -0x7F00
//I (17377) esp-tls-mbedtls: (FFFF8100): SSL - Memory allocation failed