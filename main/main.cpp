#include "iot_setup.h"
#include <Arduino.h>
#include <QMI8658.h>

extern "C" void app_main()
{
    initArduino();
    Serial.begin(115200);

    initQMI8658Operation();
    establishConnection();

    while (true)
    {
        delay(portMAX_DELAY);
    }
}
