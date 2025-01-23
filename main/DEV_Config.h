/*****************************************************************************
* | File      	:   DEV_Config.h
* | Author      :
* | Function    :   Hardware underlying interface
* | Info        :
*----------------
* |	This version:   V1.0
* | Date        :   2021-03-16
* | Info        :
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documnetation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of theex Software, and to permit persons to  whom the Software is
# furished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS OR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
******************************************************************************/
#ifndef _DEV_CONFIG_H_
#define _DEV_CONFIG_H_

#include "stdio.h"

#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
/**
 * data
 **/
#define UBYTE uint8_t
#define UWORD uint16_t
#define UDOUBLE uint32_t

#define SPI_PORT spi1
#define I2C_PORT i2c2

#if CONFIG_IDF_TARGET_ESP32S2 || CONFIG_IDF_TARGET_ESP32S3
#define VSPI FSPI
#endif

/**
 * GPIOI config
 **/

#define LCD_DC_PIN      (8)
#define LCD_CS_PIN      (9)
#define LCD_CLK_PIN     (10)
#define LCD_MOSI_PIN    (11)
#define LCD_MISO_PIN    (12)
#define LCD_RST_PIN     (14)
#define LCD_BL_PIN      (2)

#define DEV_SDA_PIN     (6)
#define DEV_SCL_PIN     (7)
#define DEV_INT2_PIN    (3)

#define Touch_INT_PIN   (5)
#define Touch_RST_PIN   (13)

#define BAT_ADC_PIN     (1)
// #define BAR_CHANNEL     (A3)

#endif
