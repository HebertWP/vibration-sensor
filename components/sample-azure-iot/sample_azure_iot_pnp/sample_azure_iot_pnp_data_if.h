/* Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License. */

/**
 * @brief Defines an interface to be used by samples when interacting with sample_azure_iot_pnp.c module.
 *        This interface allows the module implementing the specific plug-and-play model to exchange data
 *        with the module responsible for communicating with Azure IoT Hub.
 */

#ifndef SAMPLE_AZURE_IOT_PNP_DATA_IF_H
#define SAMPLE_AZURE_IOT_PNP_DATA_IF_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

#include "azure_iot_hub_client_properties.h"
#include "demo_config.h"

extern AzureIoTHubClient_t xAzureIoTHubClient;

/**
 * @brief Provides the payload to be sent as reported properties update to the Azure IoT Hub.
 *
 * @remark This function must be implemented by the specific sample.
 *         `ulCreateReportedPropertiesUpdate` is called periodically by the sample
 *         core task (the task created by `vStartDemoTask`).
 *         If the sample does not have any properties to update, just return zero to inform no
 *         update should be sent.
 *
 * @param[out] pucPropertiesData    Pointer to uint8_t* that will contain the reported properties payload.
 * @param[in]  ulPropertiesDataSize Size of `pucPropertiesData`
 *
 * @return uint32_t The number of bytes written in `pucPropertiesData`.
 */
uint32_t ulCreateReportedPropertiesUpdate( uint8_t * pucPropertiesData,
                                           uint32_t ulPropertiesDataSize );

/**
 * @brief Handles a properties message received from the Azure IoT Hub (writable or get response).
 *
 * @remark This function must be implemented by the specific sample.
 *
 * @param[in]  pxMessage                               Pointer to a structure that holds the Writable Properties received.
 * @param[out] pucWritablePropertyResponseBuffer       Buffer where to write the response for the property update.
 * @param[out] ulWritablePropertyResponseBufferSize    Size of `pucWritablePropertyResponseBuffer`.
 * @param[out] pulWritablePropertyResponseBufferLength Number of bytes written into `pucWritablePropertyResponseBuffer`.
 */
void vHandleWritableProperties( AzureIoTHubClientPropertiesResponse_t * pxMessage,
                                uint8_t * pucWritablePropertyResponseBuffer,
                                uint32_t ulWritablePropertyResponseBufferSize,
                                uint32_t * pulWritablePropertyResponseBufferLength );
#ifdef __cplusplus
}
#endif
#endif /* ifndef SAMPLE_AZURE_IOT_PNP_DATA_IF_H */

