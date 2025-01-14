#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include "SensorQMI8658.hpp"
#include "arduinoFFT.h"

#define SENSOR_SDA  6
#define SENSOR_SCL  7
#define SENSOR_INT2  3

SensorQMI8658 qmi;

IMUdata acc;
IMUdata gyr;

const uint_fast16_t buffer_size = 128;
IMUdata accel[buffer_size];
float vReal[buffer_size];
float vImag[buffer_size];
float fftAccX[buffer_size];
IMUdata gyro[buffer_size];
bool disable_fifo = false;
uint32_t timestamp = 0;
float frequency = 896.8;
ArduinoFFT<float> FFT = ArduinoFFT<float>(vReal, vImag, buffer_size, frequency);


TaskHandle_t xHandle = NULL;
//Add locked to not read and write at the same time
SemaphoreHandle_t xMutex = xSemaphoreCreateMutex();
void vTaskFunction(void *pvParameters)
{
    while(true)
    {
        xSemaphoreTake(xMutex, portMAX_DELAY);
        uint16_t samples_num = qmi.readFromFifo(accel, buffer_size, gyro, buffer_size);
        ESP_LOGI("QMI8658", "Samples: %d", samples_num);
        for(int i = 0; i < samples_num; i++)
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

extern "C" void app_main()
{
    initArduino();
    
    Serial.begin(115200);
    
    qmi.setPins(SENSOR_INT2);
    
    if (!qmi.begin(Wire, QMI8658_L_SLAVE_ADDRESS, SENSOR_SDA, SENSOR_SCL)) {
        ESP_LOGE("QMI8658", "Failed to find QMI8658 - check your wiring!");
        ESP.restart();
    }
    //set wire buffer size
    Wire.setBufferSize(2048);
    //set frequency
    Wire.setClock(400000);
    ESP_LOGI("QMI8658", "Found QMI8658, Device ID: 0x%02X", qmi.getChipID());

    qmi.configAccelerometer(SensorQMI8658::ACC_RANGE_4G, SensorQMI8658::ACC_ODR_125Hz, SensorQMI8658::LPF_MODE_3);
    //qmi.configGyroscope(SensorQMI8658::GYR_RANGE_64DPS, SensorQMI8658::GYR_ODR_896_8Hz, SensorQMI8658::LPF_MODE_0);

    attachInterrupt(SENSOR_INT2, gpio_isr_handler, RISING);

    qmi.configFIFO(SensorQMI8658::FIFO_MODE_FIFO, SensorQMI8658::FIFO_SAMPLES_128, SensorQMI8658::INTERRUPT_PIN_2, 128);

    /*
    * If both the accelerometer and gyroscope sensors are turned on at the same time,
    * the output frequency will be based on the gyroscope output frequency.
    * The example configuration is 896.8HZ output frequency,
    * so the acceleration output frequency is also limited to 896.8HZ
    * */

    qmi.enableAccelerometer();
    //qmi.enableGyroscope();

    qmi.enableINT(SensorQMI8658::INTERRUPT_PIN_1, false);
    qmi.enableINT(SensorQMI8658::INTERRUPT_PIN_2, true);
    pinMode(SENSOR_INT2, INPUT);

    xTaskCreate(vTaskFunction, "Task", 20024, NULL, 1, &xHandle);

    while( true)
    {
        xSemaphoreTake(xMutex, portMAX_DELAY);
        for(int i = 0; i < buffer_size; i++)
        {
            Serial.printf("%.2f,", vReal[i]);
        }
        Serial.print("\n");
        xSemaphoreGive(xMutex);
        delay(20000);
    }
}
