#include "Arduino.h"
#include <Wire.h>

#include "QMI8658.h"
#include "arduinoFFT.h"
#include "SensorQMI8658.hpp"
#include "DEV_Config.h"

SensorQMI8658 qmi;

#define FREQUENCY 1000
#define GRAVITY 9.81

SemaphoreHandle_t xMutex = xSemaphoreCreateMutex();
SemaphoreHandle_t canRead = xSemaphoreCreateMutex();

IMUdata acceleration[TOTAL_READS];
TaskHandle_t readDataHandle = NULL;
TaskHandle_t calculateFFTHandle = NULL;

float vImag[TOTAL_READS];
float reads[TOTAL_READS];

ArduinoFFT<float> FFT = ArduinoFFT<float>(reads, vImag, TOTAL_READS, FREQUENCY);

void vTaskReadDataFromSensorBuffer(void *pvParameters)
{
    uint8_t inRead = 0;
    while (true)
    {
        vTaskSuspend(NULL);
        if (inRead == 0)
        {
            xSemaphoreTake(xMutex, portMAX_DELAY);
        }
        qmi.readFromFifo(&acceleration[inRead * SAMPLES_NUM], SAMPLES_NUM, NULL, 0);
        inRead++;
        if (inRead == NUM_READS)
        {
            inRead = 0;
            xSemaphoreGive(xMutex);
        }
    }
}

void vTaskCalculatedFFT(void *pvParameters)
{
    while (true)
    {
        xSemaphoreTake(xMutex, portMAX_DELAY);
        xSemaphoreTake(canRead, portMAX_DELAY);
        for (int i = 0; i < TOTAL_READS; i++)
        {
            reads[i] = acceleration[i].z * GRAVITY;
            vImag[i] = 0;
        }
        xSemaphoreGive(xMutex);
        FFT.compute(FFTDirection::Forward);
        FFT.complexToMagnitude();
        xSemaphoreGive(canRead);
        vTaskDelay(30000 / portTICK_PERIOD_MS);
    }
}

void IRAM_ATTR gpio_isr_handler()
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xTaskResumeFromISR(readDataHandle);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

extern void initQMI8658Operation()
{
    FFT.windowing(FFTWindow::Blackman_Harris, FFTDirection::Forward);

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

    qmi.configFIFO(SensorQMI8658::FIFO_MODE_FIFO, SensorQMI8658::FIFO_SAMPLES_128, SensorQMI8658::INTERRUPT_PIN_2, SAMPLES_NUM);

    qmi.enableAccelerometer();
    
    qmi.enableINT(SensorQMI8658::INTERRUPT_PIN_1, false);
    qmi.enableINT(SensorQMI8658::INTERRUPT_PIN_2, true);
    pinMode(DEV_INT2_PIN, INPUT);
    xTaskCreate(vTaskReadDataFromSensorBuffer, "ReadTask", 20024, NULL, 1, &readDataHandle);
    xTaskCreatePinnedToCore(vTaskCalculatedFFT, "FFTTask", 20024, NULL, 1, &calculateFFTHandle, 0);
}
