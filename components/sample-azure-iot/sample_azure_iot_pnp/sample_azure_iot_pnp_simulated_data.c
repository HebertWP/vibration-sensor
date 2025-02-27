/* Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License. */

#include "sample_azure_iot_pnp_data_if.h"

/* Standard includes. */
#include <string.h>
#include <stdio.h>

/* Azure JSON includes */
#include "azure_iot_json_reader.h"
#include "azure_iot_json_writer.h"

/* FreeRTOS */
/* This task provides taskDISABLE_INTERRUPTS, used by configASSERT */
#include "FreeRTOS.h"
#include "task.h"

/**
 * @brief Command values
 */
#define sampleazureiotCOMMAND_MAX_MIN_REPORT              "getMaxMinReport"
#define sampleazureiotCOMMAND_MAX_TEMP                    "maxTemp"
#define sampleazureiotCOMMAND_MIN_TEMP                    "minTemp"
#define sampleazureiotCOMMAND_TEMP_VERSION                "avgTemp"
#define sampleazureiotCOMMAND_START_TIME                  "startTime"
#define sampleazureiotCOMMAND_END_TIME                    "endTime"
#define sampleazureiotCOMMAND_EMPTY_PAYLOAD               "{}"
#define sampleazureiotCOMMAND_FAKE_END_TIME               "2023-01-10T10:00:00Z"

/**
 * @brief Device values
 */
#define sampleazureiotDEFAULT_START_TEMP_COUNT            1
#define sampleazureiotDEFAULT_START_TEMP_CELSIUS          22.0
#define sampleazureiotDOUBLE_DECIMAL_PLACE_DIGITS         2

/**
 * @brief Property Values
 */
#define sampleazureiotPROPERTY_STATUS_SUCCESS             200
#define sampleazureiotPROPERTY_SUCCESS                    "success"
#define sampleazureiotPROPERTY_TARGET_TEMPERATURE_TEXT    "targetTemperature"

/**
 * @brief Telemetry values
 */
#define sampleazureiotTELEMETRY_NAME                      "temperature"

/**
 *@brief The Telemetry message published in this example.
 */
#define sampleazureiotMESSAGE                             "{\"" sampleazureiotTELEMETRY_NAME "\":%0.2f}"


/* Device values */
static double xDeviceCurrentTemperature = sampleazureiotDEFAULT_START_TEMP_CELSIUS;
static double xDeviceMaximumTemperature = sampleazureiotDEFAULT_START_TEMP_CELSIUS;
static double xDeviceMinimumTemperature = sampleazureiotDEFAULT_START_TEMP_CELSIUS;
static double xDeviceTemperatureSummation = sampleazureiotDEFAULT_START_TEMP_CELSIUS;
static uint32_t ulDeviceTemperatureCount = sampleazureiotDEFAULT_START_TEMP_COUNT;
static double xDeviceAverageTemperature = sampleazureiotDEFAULT_START_TEMP_CELSIUS;

/* Command buffers */
static uint8_t ucCommandStartTimeValueBuffer[ 32 ];
/*-----------------------------------------------------------*/

static void prvSkipPropertyAndValue( AzureIoTJSONReader_t * pxReader )
{
    AzureIoTResult_t xResult;

    xResult = AzureIoTJSONReader_NextToken( pxReader );
    configASSERT( xResult == eAzureIoTSuccess );

    xResult = AzureIoTJSONReader_SkipChildren( pxReader );
    configASSERT( xResult == eAzureIoTSuccess );

    xResult = AzureIoTJSONReader_NextToken( pxReader );
    configASSERT( xResult == eAzureIoTSuccess );
}
/*-----------------------------------------------------------*/

/**
 * @brief Update local device temperature values based on new requested temperature.
 */
static void prvUpdateLocalProperties( double xNewTemperatureValue,
                                      uint32_t ulPropertyVersion,
                                      bool * pxOutMaxTempChanged )
{
    *pxOutMaxTempChanged = false;
    xDeviceCurrentTemperature = xNewTemperatureValue;

    /* Update maximum or minimum temperatures. */
    if( xDeviceCurrentTemperature > xDeviceMaximumTemperature )
    {
        xDeviceMaximumTemperature = xDeviceCurrentTemperature;
        *pxOutMaxTempChanged = true;
    }
    else if( xDeviceCurrentTemperature < xDeviceMinimumTemperature )
    {
        xDeviceMinimumTemperature = xDeviceCurrentTemperature;
    }

    /* Calculate the new average temperature. */
    ulDeviceTemperatureCount++;
    xDeviceTemperatureSummation += xDeviceCurrentTemperature;
    xDeviceAverageTemperature = xDeviceTemperatureSummation / ulDeviceTemperatureCount;

    LogInfo( ( "Client updated desired temperature variables locally." ) );
    LogInfo( ( "Current Temperature: %2f", xDeviceCurrentTemperature ) );
    LogInfo( ( "Maximum Temperature: %2f", xDeviceMaximumTemperature ) );
    LogInfo( ( "Minimum Temperature: %2f", xDeviceMinimumTemperature ) );
    LogInfo( ( "Average Temperature: %2f", xDeviceAverageTemperature ) );
}
/*-----------------------------------------------------------*/

/**
 * @brief Generate an update for the device's target temperature property online,
 *        acknowledging the update from the IoT Hub.
 */
static uint32_t prvGenerateAckForIncomingTemperature( double xUpdatedTemperature,
                                                      uint32_t ulVersion,
                                                      uint8_t * pucResponseBuffer,
                                                      uint32_t ulResponseBufferSize )
{
    AzureIoTResult_t xResult;
    AzureIoTJSONWriter_t xWriter;
    int32_t lBytesWritten;

    /* Building the acknowledgement payload for the temperature property to signal we successfully received and accept it. */
    xResult = AzureIoTJSONWriter_Init( &xWriter, pucResponseBuffer, ulResponseBufferSize );
    configASSERT( xResult == eAzureIoTSuccess );

    xResult = AzureIoTJSONWriter_AppendBeginObject( &xWriter );
    configASSERT( xResult == eAzureIoTSuccess );

    xResult = AzureIoTHubClientProperties_BuilderBeginResponseStatus( &xAzureIoTHubClient,
                                                                      &xWriter,
                                                                      ( const uint8_t * ) sampleazureiotPROPERTY_TARGET_TEMPERATURE_TEXT,
                                                                      sizeof( sampleazureiotPROPERTY_TARGET_TEMPERATURE_TEXT ) - 1,
                                                                      sampleazureiotPROPERTY_STATUS_SUCCESS,
                                                                      ulVersion,
                                                                      ( const uint8_t * ) sampleazureiotPROPERTY_SUCCESS,
                                                                      sizeof( sampleazureiotPROPERTY_SUCCESS ) - 1 );
    configASSERT( xResult == eAzureIoTSuccess );

    xResult = AzureIoTJSONWriter_AppendDouble( &xWriter, xUpdatedTemperature, sampleazureiotDOUBLE_DECIMAL_PLACE_DIGITS );
    configASSERT( xResult == eAzureIoTSuccess );

    xResult = AzureIoTHubClientProperties_BuilderEndResponseStatus( &xAzureIoTHubClient,
                                                                    &xWriter );
    configASSERT( xResult == eAzureIoTSuccess );

    xResult = AzureIoTJSONWriter_AppendEndObject( &xWriter );
    configASSERT( xResult == eAzureIoTSuccess );

    lBytesWritten = AzureIoTJSONWriter_GetBytesUsed( &xWriter );
    configASSERT( lBytesWritten > 0 );

    return ( uint32_t ) lBytesWritten;
}
/*-----------------------------------------------------------*/

/**
 * @brief Properties callback handler
 */
static AzureIoTResult_t prvProcessProperties( AzureIoTHubClientPropertiesResponse_t * pxMessage,
                                              AzureIoTHubClientPropertyType_t xPropertyType,
                                              double * pxOutTemperature,
                                              uint32_t * ulOutVersion )
{
    AzureIoTResult_t xResult;
    AzureIoTJSONReader_t xReader;
    const uint8_t * pucComponentName = NULL;
    uint32_t ulComponentNameLength = 0;

    *pxOutTemperature = 0.0;

    xResult = AzureIoTJSONReader_Init( &xReader, pxMessage->pvMessagePayload, pxMessage->ulPayloadLength );
    configASSERT( xResult == eAzureIoTSuccess );

    xResult = AzureIoTHubClientProperties_GetPropertiesVersion( &xAzureIoTHubClient, &xReader, pxMessage->xMessageType, ulOutVersion );

    if( xResult != eAzureIoTSuccess )
    {
        LogError( ( "Error getting the property version: result 0x%08x", xResult ) );
    }
    else
    {
        /* Reset JSON reader to the beginning */
        xResult = AzureIoTJSONReader_Init( &xReader, pxMessage->pvMessagePayload, pxMessage->ulPayloadLength );
        configASSERT( xResult == eAzureIoTSuccess );

        while( ( xResult = AzureIoTHubClientProperties_GetNextComponentProperty( &xAzureIoTHubClient, &xReader,
                                                                                 pxMessage->xMessageType, xPropertyType,
                                                                                 &pucComponentName, &ulComponentNameLength ) ) == eAzureIoTSuccess )
        {
            if( ulComponentNameLength > 0 )
            {
                LogInfo( ( "Unknown component name received" ) );

                /* Unknown component name arrived (there are none for this device).
                 * We have to skip over the property and value to continue iterating */
                prvSkipPropertyAndValue( &xReader );
            }
            else if( AzureIoTJSONReader_TokenIsTextEqual( &xReader,
                                                          ( const uint8_t * ) sampleazureiotPROPERTY_TARGET_TEMPERATURE_TEXT,
                                                          sizeof( sampleazureiotPROPERTY_TARGET_TEMPERATURE_TEXT ) - 1 ) )
            {
                xResult = AzureIoTJSONReader_NextToken( &xReader );
                configASSERT( xResult == eAzureIoTSuccess );

                /* Get desired temperature */
                xResult = AzureIoTJSONReader_GetTokenDouble( &xReader, pxOutTemperature );

                if( xResult != eAzureIoTSuccess )
                {
                    LogError( ( "Error getting the property version: result 0x%08x", xResult ) );
                    break;
                }

                xResult = AzureIoTJSONReader_NextToken( &xReader );
                configASSERT( xResult == eAzureIoTSuccess );
            }
            else
            {
                LogInfo( ( "Unknown property arrived: skipping over it." ) );

                /* Unknown property arrived. We have to skip over the property and value to continue iterating. */
                prvSkipPropertyAndValue( &xReader );
            }
        }

        if( xResult != eAzureIoTErrorEndOfProperties )
        {
            LogError( ( "There was an error parsing the properties: result 0x%08x", xResult ) );
        }
        else
        {
            LogInfo( ( "Successfully parsed properties" ) );
            xResult = eAzureIoTSuccess;
        }
    }

    return xResult;
}
/*-----------------------------------------------------------*/

/**
 * @brief Property message callback handler
 */
void vHandleWritableProperties( AzureIoTHubClientPropertiesResponse_t * pxMessage,
                                uint8_t * pucWritablePropertyResponseBuffer,
                                uint32_t ulWritablePropertyResponseBufferSize,
                                uint32_t * pulWritablePropertyResponseBufferLength )
{
    AzureIoTResult_t xResult;
    double xIncomingTemperature;
    uint32_t ulVersion;
    bool xWasMaxTemperatureChanged = false;

    xResult = prvProcessProperties( pxMessage, eAzureIoTHubClientPropertyWritable, &xIncomingTemperature, &ulVersion );

    if( xResult == eAzureIoTSuccess )
    {
        prvUpdateLocalProperties( xIncomingTemperature, ulVersion, &xWasMaxTemperatureChanged );
        *pulWritablePropertyResponseBufferLength = prvGenerateAckForIncomingTemperature(
            xIncomingTemperature,
            ulVersion,
            pucWritablePropertyResponseBuffer,
            ulWritablePropertyResponseBufferSize );
    }
    else
    {
        LogError( ( "There was an error processing incoming properties: result 0x%08x", xResult ) );
    }
}
/*-----------------------------------------------------------*/