#include <Arduino.h>
#include <QMI8658.h>

extern "C" void app_main()
{
    initArduino();

    Serial.begin(115200);

    initQMI8658Operation();

    while (true)
    {
        xSemaphoreTake(xMutex, portMAX_DELAY);
        for (int i = 0; i < BUFFER_SIZE; i++)
        {
            Serial.printf("%.2f,", vReal[i]);
        }
        Serial.print("\n");
        xSemaphoreGive(xMutex);
        delay(20000);
    }
}
