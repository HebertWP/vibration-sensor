/*
 * SPDX-FileCopyrightText: 2021 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/* Attributes State Machine */
enum
{
    IDX_SVC,

    IDX_CHAR_SSID,
    IDX_CHAR_VAL_SSID,
    
    IDX_CHAR_PASSWORD,
    IDX_CHAR_VAL_PASSWORD,

    HRS_IDX_NB,
};