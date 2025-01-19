#include "iot_setup.h"
#include <Arduino.h>
#include <QMI8658.h>

#define SCL_INDEX 0x00
#define SCL_TIME 0x01
#define SCL_FREQUENCY 0x02
#define SCL_PLOT 0x03

float samplingFrequency = 1000;

void PrintVector(float *vData, uint16_t bufferSize, uint8_t scaleType)
{
  for (uint16_t i = 0; i < bufferSize; i++)
  {
    double abscissa = 0;
    /* Print abscissa value */
    switch (scaleType)
    {
      case SCL_INDEX:
        abscissa = (i * 1.0);
	break;
      case SCL_TIME:
        abscissa = ((i * 1.0) / samplingFrequency);
	break;
      case SCL_FREQUENCY:
        abscissa = ((i * 1.0 * samplingFrequency) / TOTAL_READS);
	break;
    }
    Serial.print(abscissa, 6);
    if(scaleType==SCL_FREQUENCY)
      Serial.print("Hz");
    Serial.print(" ");
    Serial.println(vData[i], 4);
  }
  Serial.println();
}

extern "C" void app_main()
{
    initArduino();
    Serial.begin(115200);

    initQMI8658Operation();
    establishConnection();

    while (true)
    {
        //xSemaphoreTake(canRead, portMAX_DELAY);
        //PrintVector(reads, (TOTAL_READS >> 1), SCL_FREQUENCY);
        //xSemaphoreGive(canRead);
        delay(30000);
    }
}