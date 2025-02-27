#ifndef QMI8658_SETUP_H
#define QMI8658_SETUP_H

#include <Wire.h>
#include "SensorQMI8658.hpp"

#define SAMPLES_NUM 128
#define NUM_READS 8
#define TOTAL_READS (NUM_READS * SAMPLES_NUM)

extern float reads[TOTAL_READS];

extern SemaphoreHandle_t canRead;

extern void setupQMI8658();

#endif