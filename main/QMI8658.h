#ifndef QMI8658_H
#define QMI8658_H

#include <Wire.h>
#include "SensorQMI8658.hpp"

#define BUFFER_SIZE 128

extern float vReal[BUFFER_SIZE];
extern float vImag[BUFFER_SIZE];
extern SemaphoreHandle_t xMutex;

extern void initQMI8658Operation();

#endif // QMI8658_H