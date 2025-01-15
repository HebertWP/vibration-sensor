#include "Arduino.h"
#include <Wire.h>

#include "QMI8658.h"
#include "arduinoFFT.h"
#include "SensorQMI8658.hpp"
#include "DEV_Config.h"

SensorQMI8658 qmi;

float vReal[BUFFER_SIZE];
float vImag[BUFFER_SIZE];
SemaphoreHandle_t xMutex = xSemaphoreCreateMutex();

IMUdata accel[BUFFER_SIZE];

float frequency = 1000;
ArduinoFFT<float> FFT = ArduinoFFT<float>(vReal, vImag, (uint_fast16_t)BUFFER_SIZE, frequency);

TaskHandle_t xHandle = NULL;
// Add locked to not read and write at the same time
void vTaskFunction(void *pvParameters)
{
    while (true)
    {
        xSemaphoreTake(xMutex, portMAX_DELAY);
        uint16_t samples_num = qmi.readFromFifo(accel, BUFFER_SIZE, NULL, BUFFER_SIZE);
        for (int i = 0; i < samples_num; i++)
        {
            vReal[i] = accel[i].z;
            vImag[i] = 0;
        }
        FFT.compute(FFTDirection::Forward);
        FFT.complexToMagnitude();
        xSemaphoreGive(xMutex);
        vTaskSuspend(NULL);
    }
}

void IRAM_ATTR gpio_isr_handler()
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xTaskResumeFromISR(xHandle);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

extern void initQMI8658Operation()
{
    FFT.windowing(FFTWindow::Hamming, FFTDirection::Forward);	/* Weigh data */

    qmi.setPins(DEV_INT2_PIN);

    if (!qmi.begin(Wire, QMI8658_L_SLAVE_ADDRESS, DEV_SDA_PIN, DEV_SCL_PIN))
    {
        ESP_LOGE("QMI8658", "Failed to find QMI8658 - check your wiring!");
        ESP.restart();
    }
    // set wire buffer size
    Wire.setBufferSize(2048);
    // set frequency
    Wire.setClock(400000);

    qmi.configAccelerometer(SensorQMI8658::ACC_RANGE_2G, SensorQMI8658::ACC_ODR_1000Hz, SensorQMI8658::LPF_MODE_3);

    attachInterrupt(DEV_INT2_PIN, gpio_isr_handler, RISING);

    qmi.configFIFO(SensorQMI8658::FIFO_MODE_FIFO, SensorQMI8658::FIFO_SAMPLES_128, SensorQMI8658::INTERRUPT_PIN_2, BUFFER_SIZE);

    qmi.enableAccelerometer();
    
    qmi.enableINT(SensorQMI8658::INTERRUPT_PIN_1, false);
    qmi.enableINT(SensorQMI8658::INTERRUPT_PIN_2, true);
    pinMode(DEV_INT2_PIN, INPUT);
    xTaskCreate(vTaskFunction, "Task", 20024, NULL, 1, &xHandle);
}
