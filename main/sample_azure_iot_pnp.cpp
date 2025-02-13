/* Copyright (c) Microsoft Corporation.
 * Licensed under the MIT License. */

/* Standard includes. */
#include <string.h>
#include <stdio.h>

/* Kernel includes. */
#include "FreeRTOS.h"
#include "task.h"

/* Azure Provisioning/IoT Hub library includes */
#include "azure_iot_hub_client.h"
#include "azure_iot_hub_client_properties.h"
#include "azure_iot_provisioning_client.h"

/* Azure JSON includes */
#include "azure_iot_json_reader.h"
#include "azure_iot_json_writer.h"

/* Exponential backoff retry include. */
#include "backoff_algorithm.h"

/* Transport interface implementation include header for TLS. */
#include "transport_tls_socket.h"

/* Crypto helper header. */
#include "azure_sample_crypto.h"

/* Demo Specific configs. */
#include "demo_config.h"

/* Demo Specific Interface Functions. */
#include "azure_sample_connection.h"

/* Data Interface Definition */
#include "sample_azure_iot_pnp_data_if.h"

#include "wifi_setup.h"
#include "device_configuration.h"
#include <ArduinoJson.h>
#include "QMI8658_setup.h"

/**
 * @brief The maximum number of retries for network operation with server.
 */
#define sampleazureiotRETRY_MAX_ATTEMPTS (5U)

/**
 * @brief The maximum back-off delay (in milliseconds) for retrying failed operation
 *  with server.
 */
#define sampleazureiotRETRY_MAX_BACKOFF_DELAY_MS (5000U)

/**
 * @brief The base back-off delay (in milliseconds) to use for network operation retry
 * attempts.
 */
#define sampleazureiotRETRY_BACKOFF_BASE_MS (500U)

/**
 * @brief Timeout for receiving CONNACK packet in milliseconds.
 */
#define sampleazureiotCONNACK_RECV_TIMEOUT_MS (10 * 1000U)

/**
 * @brief Date-time to use for the model id
 */
#define sampleazureiotDATE_TIME_FORMAT "%Y-%m-%dT%H:%M:%S.000Z"

/**
 * @brief Time in ticks to wait between each cycle of the demo implemented
 * by prvMQTTDemoTask().
 */
#define sampleazureiotDELAY_BETWEEN_DEMO_ITERATIONS_TICKS (pdMS_TO_TICKS(5000U))

/**
 * @brief Timeout for MQTT_ProcessLoop in milliseconds.
 */
#define sampleazureiotPROCESS_LOOP_TIMEOUT_MS (500U)

/**
 * @brief Delay (in ticks) between consecutive cycles of MQTT publish operations in a
 * demo iteration.
 *
 * Note that the process loop also has a timeout, so the total time between
 * publishes is the sum of the two delays.
 */
#define sampleazureiotDELAY_BETWEEN_PUBLISHES_TICKS (pdMS_TO_TICKS(2000U))

/**
 * @brief Transport timeout in milliseconds for transport send and receive.
 */
#define sampleazureiotTRANSPORT_SEND_RECV_TIMEOUT_MS (2000U)

/**
 * @brief Transport timeout in milliseconds for transport send and receive.
 */
#define sampleazureiotProvisioning_Registration_TIMEOUT_MS (3 * 1000U)

/**
 * @brief Wait timeout for subscribe to finish.
 */
#define sampleazureiotSUBSCRIBE_TIMEOUT (10 * 1000U)
/*-----------------------------------------------------------*/

#define DOUBLE_DECIMAL_PLACE_DIGITS 2
#define REBOOT_COMAND "reboot"

char g_certificate[2048];
char g_key[2048];

/* Each compilation unit must define the NetworkContext struct. */
struct NetworkContext
{
    void *pParams;
};

AzureIoTHubClient_t xAzureIoTHubClient;

/* Telemetry buffers */
static uint8_t ucScratchBuffer[4098];

/* Command buffers */
static uint8_t ucCommandResponsePayloadBuffer[256];

/* Reported Properties buffers */
static uint8_t ucReportedPropertiesUpdate[380];
static uint32_t ulReportedPropertiesUpdateLength;
/*-----------------------------------------------------------*/

/**
 * @brief The task used to demonstrate the Azure IoT Hub API.
 *
 * @param[in] pvParameters Parameters as passed at the time of task creation. Not
 * used in this example.
 */
static void prvAzureDemoTask(void *pvParameters);

/**
 * @brief Connect to endpoint with reconnection retries.
 *
 * If connection fails, retry is attempted after a timeout.
 * Timeout value will exponentially increase until maximum
 * timeout value is reached or the number of attempts are exhausted.
 *
 * @param pcHostName Hostname of the endpoint to connect to.
 * @param ulPort Endpoint port.
 * @param pxNetworkCredentials Pointer to Network credentials.
 * @param pxNetworkContext Point to Network context created.
 * @return uint32_t The status of the final connection attempt.
 */
static uint32_t prvConnectToServerWithBackoffRetries(const char *pcHostName,
                                                     uint32_t ulPort,
                                                     NetworkCredentials_t *pxNetworkCredentials,
                                                     NetworkContext_t *pxNetworkContext);
/*-----------------------------------------------------------*/

/**
 * @brief Static buffer used to hold MQTT messages being sent and received.
 */
static uint8_t ucMQTTMessageBuffer[democonfigNETWORK_BUFFER_SIZE];

/**
 * @brief Internal function for handling Command requests.
 *
 * @remark This function is required for the interface with samples to work properly.
 */
static void prvHandleCommand(AzureIoTHubClientCommandRequest_t *pxMessage,
                             void *pvContext)
{
    AzureIoTHubClient_t *pxHandle = (AzureIoTHubClient_t *)pvContext;
    uint32_t ulResponseStatus = 0;
    AzureIoTResult_t xResult;
    ESP_LOGI("Command", "Comand name: %s", (const char *)pxMessage->pucCommandName);
    if (strncmp((const char *)pxMessage->pucCommandName, REBOOT_COMAND, strlen(REBOOT_COMAND)) == 0)
    {
        ESP_LOGI("Command", "Rebooting");
        if ((xResult = AzureIoTHubClient_SendCommandResponse(pxHandle, pxMessage, ulResponseStatus,
                                                             ucCommandResponsePayloadBuffer,
                                                             0)) != eAzureIoTSuccess)
        {
            LogError(("Error sending command response: result 0x%08x", (uint16_t)xResult));
        }
        else
        {
            LogInfo(("Successfully sent command response %d", (int16_t)ulResponseStatus));
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
        esp_restart();
    }
}

static void prvDispatchPropertiesUpdate(AzureIoTHubClientPropertiesResponse_t *pxMessage)
{
    vHandleWritableProperties(pxMessage,
                              ucReportedPropertiesUpdate,
                              sizeof(ucReportedPropertiesUpdate),
                              &ulReportedPropertiesUpdateLength);

    if (ulReportedPropertiesUpdateLength == 0)
    {
        LogError(("Failed to send response to writable properties update, length of response is zero."));
    }
    else
    {
        AzureIoTResult_t xResult = AzureIoTHubClient_SendPropertiesReported(&xAzureIoTHubClient,
                                                                            ucReportedPropertiesUpdate,
                                                                            ulReportedPropertiesUpdateLength,
                                                                            NULL);
        configASSERT(xResult == eAzureIoTSuccess);
    }
}
/*-----------------------------------------------------------*/

/**
 * @brief Private property message callback handler.
 *        This handler dispatches the calls to the functions defined in
 *        sample_azure_iot_pnp_data_if.h
 */
static void prvHandleProperties(AzureIoTHubClientPropertiesResponse_t *pxMessage,
                                void *pvContext)
{
    (void)pvContext;

    LogDebug(("Property document payload : %.*s \r\n",
              (int16_t)pxMessage->ulPayloadLength,
              (const char *)pxMessage->pvMessagePayload));

    switch (pxMessage->xMessageType)
    {
    case eAzureIoTHubPropertiesRequestedMessage:
        LogDebug(("Device property document GET received"));
        prvDispatchPropertiesUpdate(pxMessage);
        break;

    case eAzureIoTHubPropertiesWritablePropertyMessage:
        LogDebug(("Device writeable property received"));
        prvDispatchPropertiesUpdate(pxMessage);
        break;

    case eAzureIoTHubPropertiesReportedResponseMessage:
        LogDebug(("Device reported property response received"));
        break;

    default:
        LogError(("Unknown property message: 0x%08x", pxMessage->xMessageType));
        configASSERT(false);
    }
}
/*-----------------------------------------------------------*/

esp_err_t loadCert(const char *file_path, char *out)
{
    FILE *f = fopen(file_path, "r");
    char line[256];
    int index = 0;
    while (fgets(line, sizeof(line), f))
    {
        for (int i = 0; i < strlen(line); i++)
        {
            out[index++] = line[i];
        }
    }
    // out[index++] = '\n';
    out[index++] = '\0';
    out[index] = '\0';
    fclose(f);
    return ESP_OK;
}

/**
 * @brief Setup transport credentials.
 */
static uint32_t prvSetupNetworkCredentials(NetworkCredentials_t *pxNetworkCredentials)
{
    pxNetworkCredentials->xDisableSni = pdFALSE;
    /* Set the credentials for establishing a TLS connection. */
    pxNetworkCredentials->pucRootCa = (const unsigned char *)democonfigROOT_CA_PEM;
    pxNetworkCredentials->xRootCaSize = sizeof(democonfigROOT_CA_PEM);

    loadCert("/spiffs/ca.pem", g_certificate);
    loadCert("/spiffs/cert_key.key", g_key);
    pxNetworkCredentials->pucClientCert = (const unsigned char *)g_certificate;
    pxNetworkCredentials->xClientCertSize = strlen(g_certificate) + 1;
    pxNetworkCredentials->pucPrivateKey = (const unsigned char *)g_key;
    pxNetworkCredentials->xPrivateKeySize = strlen(g_key) + 1;
    return 0;
}
/*-----------------------------------------------------------*/

uint32_t generateTelemetryPayload(
    uint8_t *pucTelemetryData,
    uint32_t ulTelemetryDataSize,
    uint32_t *ulTelemetryDataLength)
{
    xSemaphoreTake(canRead, portMAX_DELAY);
    JsonDocument doc;
    for (int i = 0; i < 128; i++)
    {
        doc["FFT"][i] = reads[i];
    }
    xSemaphoreGive(canRead);
    *ulTelemetryDataLength = serializeJson(doc, (char *)pucTelemetryData, ulTelemetryDataSize);
    return ESP_OK;
}

/**
 * @brief Implements the sample interface for generating reported properties payload.
 */
uint32_t createReportedPropertiesUpdate(uint8_t *pucPropertiesData,
                                        uint32_t ulPropertiesDataSize)
{
    AzureIoTResult_t xResult;
    AzureIoTJSONWriter_t xWriter;
    int32_t lBytesWritten;

    /* Initialize the JSON writer with the buffer to which we will write the payload with the new temperature. */
    xResult = AzureIoTJSONWriter_Init(&xWriter, pucPropertiesData, ulPropertiesDataSize);
    configASSERT(xResult == eAzureIoTSuccess);

    xResult = AzureIoTJSONWriter_AppendBeginObject(&xWriter);
    configASSERT(xResult == eAzureIoTSuccess);

    xResult = AzureIoTJSONWriter_AppendPropertyName(&xWriter, (const uint8_t *)"samplingFrequency",
                                                    sizeof("samplingFrequency") - 1);
    configASSERT(xResult == eAzureIoTSuccess);

    xResult = AzureIoTJSONWriter_AppendDouble(&xWriter, 1000, DOUBLE_DECIMAL_PLACE_DIGITS);
    configASSERT(xResult == eAzureIoTSuccess);

    xResult = AzureIoTJSONWriter_AppendEndObject(&xWriter);
    configASSERT(xResult == eAzureIoTSuccess);

    lBytesWritten = AzureIoTJSONWriter_GetBytesUsed(&xWriter);
    configASSERT(lBytesWritten > 0);

    return lBytesWritten;
}

/**
 * @brief Azure IoT demo task that gets started in the platform specific project.
 *  In this demo task, middleware API's are used to connect to Azure IoT Hub and
 *  function to adhere to the Plug and Play device convention.
 */
static void prvAzureDemoTask(void *pvParameters)
{
    uint32_t ulScratchBufferLength = 0U;
    NetworkCredentials_t xNetworkCredentials = {0};
    AzureIoTTransportInterface_t xTransport;
    NetworkContext_t xNetworkContext = {0};
    TlsTransportParams_t xTlsTransportParams = {0};
    AzureIoTResult_t xResult;
    uint32_t ulStatus;
    AzureIoTHubClientOptions_t xHubOptions = {0};
    bool xSessionPresent;

    const char *device_name = (const char *)(*g_device_document)["device_id"];
    const char *host = (const char *)(*g_device_document)["host"];

    uint8_t *pucIotHubHostname = (uint8_t *)host;
    uint8_t *pucIotHubDeviceId = (uint8_t *)device_name;
    uint32_t pulIothubHostnameLength = strlen(host);
    uint32_t pulIothubDeviceIdLength = strlen(device_name);

    (void)pvParameters;

    /* Initialize Azure IoT Middleware.  */
    configASSERT(AzureIoT_Init() == eAzureIoTSuccess);

    ulStatus = prvSetupNetworkCredentials(&xNetworkCredentials);
    configASSERT(ulStatus == 0);

    xNetworkContext.pParams = &xTlsTransportParams;

    for (;;)
    {
        if (xAzureSample_IsConnectedToInternet())
        {
            /* Attempt to establish TLS session with IoT Hub. If connection fails,
             * retry after a timeout. Timeout value will be exponentially increased
             * until  the maximum number of attempts are reached or the maximum timeout
             * value is reached. The function returns a failure status if the TCP
             * connection cannot be established to the IoT Hub after the configured
             * number of attempts. */
            ulStatus = prvConnectToServerWithBackoffRetries((const char *)pucIotHubHostname,
                                                            democonfigIOTHUB_PORT,
                                                            &xNetworkCredentials, &xNetworkContext);
            configASSERT(ulStatus == 0);

            /* Fill in Transport Interface send and receive function pointers. */
            xTransport.pxNetworkContext = &xNetworkContext;
            xTransport.xSend = TLS_Socket_Send;
            xTransport.xRecv = TLS_Socket_Recv;

            /* Init IoT Hub option */
            xResult = AzureIoTHubClient_OptionsInit(&xHubOptions);
            configASSERT(xResult == eAzureIoTSuccess);

            xHubOptions.pucModuleID = (const uint8_t *)"";
            xHubOptions.ulModuleIDLength = 0;
            xHubOptions.pucModelID = (const uint8_t *)sampleazureiotMODEL_ID;
            xHubOptions.ulModelIDLength = sizeof(sampleazureiotMODEL_ID) - 1;

            xResult = AzureIoTHubClient_Init(&xAzureIoTHubClient,
                                             pucIotHubHostname, pulIothubHostnameLength,
                                             pucIotHubDeviceId, pulIothubDeviceIdLength,
                                             &xHubOptions,
                                             ucMQTTMessageBuffer, sizeof(ucMQTTMessageBuffer),
                                             ullGetUnixTime,
                                             &xTransport);
            configASSERT(xResult == eAzureIoTSuccess);

            /* Sends an MQTT Connect packet over the already established TLS connection,
             * and waits for connection acknowledgment (CONNACK) packet. */
            LogInfo(("Creating an MQTT connection to %s.\r\n", pucIotHubHostname));

            xResult = AzureIoTHubClient_Connect(&xAzureIoTHubClient,
                                                false, &xSessionPresent,
                                                sampleazureiotCONNACK_RECV_TIMEOUT_MS);
            configASSERT(xResult == eAzureIoTSuccess);

            xResult = AzureIoTHubClient_SubscribeCommand(&xAzureIoTHubClient, prvHandleCommand,
                                                         &xAzureIoTHubClient, sampleazureiotSUBSCRIBE_TIMEOUT);
            configASSERT(xResult == eAzureIoTSuccess);

            xResult = AzureIoTHubClient_SubscribeProperties(&xAzureIoTHubClient, prvHandleProperties,
                                                            &xAzureIoTHubClient, sampleazureiotSUBSCRIBE_TIMEOUT);
            configASSERT(xResult == eAzureIoTSuccess);

            /* Get property document after initial connection */
            xResult = AzureIoTHubClient_RequestPropertiesAsync(&xAzureIoTHubClient);
            configASSERT(xResult == eAzureIoTSuccess);

            /* Publish messages with QoS1, send and process Keep alive messages. */
            for (; xAzureSample_IsConnectedToInternet();)
            {
                /* Hook for sending Telemetry */
                if ((generateTelemetryPayload(ucScratchBuffer, sizeof(ucScratchBuffer), &ulScratchBufferLength) == ESP_OK) &&
                    (ulScratchBufferLength > 0))
                {
                    ESP_LOGI("Telemetry", "%s, length: %d", ucScratchBuffer, (int)ulScratchBufferLength);
                    xResult = AzureIoTHubClient_SendTelemetry(&xAzureIoTHubClient,
                                                              ucScratchBuffer, ulScratchBufferLength,
                                                              NULL, eAzureIoTHubMessageQoS1, NULL);
                    configASSERT(xResult == eAzureIoTSuccess);
                }
                else
                {
                    ESP_LOGE("Telemetry", "Failed to generate telemetry data or length is zero. %d", (int)ulScratchBufferLength);
                }

                /* Hook for sending update to reported properties */
                ulReportedPropertiesUpdateLength = createReportedPropertiesUpdate(ucReportedPropertiesUpdate, sizeof(ucReportedPropertiesUpdate));

                if (ulReportedPropertiesUpdateLength > 0)
                {
                    xResult = AzureIoTHubClient_SendPropertiesReported(&xAzureIoTHubClient, ucReportedPropertiesUpdate, ulReportedPropertiesUpdateLength, NULL);
                    configASSERT(xResult == eAzureIoTSuccess);
                }

                for (int i = 0; i < 30; i++)
                {
                    LogInfo(("Attempt to receive publish message from IoT Hub.\r\n"));
                    xResult = AzureIoTHubClient_ProcessLoop(&xAzureIoTHubClient,
                                                            sampleazureiotPROCESS_LOOP_TIMEOUT_MS);
                    configASSERT(xResult == eAzureIoTSuccess);
                    vTaskDelay(sampleazureiotDELAY_BETWEEN_PUBLISHES_TICKS);
                }
            }

            if (xAzureSample_IsConnectedToInternet())
            {
                xResult = AzureIoTHubClient_UnsubscribeProperties(&xAzureIoTHubClient);
                configASSERT(xResult == eAzureIoTSuccess);

                xResult = AzureIoTHubClient_UnsubscribeCommand(&xAzureIoTHubClient);
                configASSERT(xResult == eAzureIoTSuccess);

                /* Send an MQTT Disconnect packet over the already connected TLS over
                 * TCP connection. There is no corresponding response for the disconnect
                 * packet. After sending disconnect, client must close the network
                 * connection. */
                xResult = AzureIoTHubClient_Disconnect(&xAzureIoTHubClient);
                configASSERT(xResult == eAzureIoTSuccess);
            }

            /* Close the network connection.  */
            TLS_Socket_Disconnect(&xNetworkContext);

            /* Wait for some time between two iterations to ensure that we do not
             * bombard the IoT Hub. */
            LogInfo(("Demo completed successfully.\r\n"));
        }

        LogInfo(("Short delay before starting the next iteration.... \r\n\r\n"));
        vTaskDelay(sampleazureiotDELAY_BETWEEN_DEMO_ITERATIONS_TICKS);
    }
}

/**
 * @brief Connect to server with backoff retries.
 */
static uint32_t prvConnectToServerWithBackoffRetries(const char *pcHostName,
                                                     uint32_t port,
                                                     NetworkCredentials_t *pxNetworkCredentials,
                                                     NetworkContext_t *pxNetworkContext)
{
    TlsTransportStatus_t xNetworkStatus;
    BackoffAlgorithmStatus_t xBackoffAlgStatus = BackoffAlgorithmSuccess;
    BackoffAlgorithmContext_t xReconnectParams;
    uint16_t usNextRetryBackOff = 0U;

    /* Initialize reconnect attempts and interval. */
    BackoffAlgorithm_InitializeParams(&xReconnectParams,
                                      sampleazureiotRETRY_BACKOFF_BASE_MS,
                                      sampleazureiotRETRY_MAX_BACKOFF_DELAY_MS,
                                      sampleazureiotRETRY_MAX_ATTEMPTS);

    /* Attempt to connect to IoT Hub. If connection fails, retry after
     * a timeout. Timeout value will exponentially increase till maximum
     * attempts are reached.
     */
    do
    {
        LogInfo(("Creating a TLS connection to %s:%u.\r\n", pcHostName, (uint16_t)port));
        /* Attempt to create a mutually authenticated TLS connection. */
        xNetworkStatus = TLS_Socket_Connect(pxNetworkContext,
                                            pcHostName, port,
                                            pxNetworkCredentials,
                                            sampleazureiotTRANSPORT_SEND_RECV_TIMEOUT_MS,
                                            sampleazureiotTRANSPORT_SEND_RECV_TIMEOUT_MS);

        if (xNetworkStatus != eTLSTransportSuccess)
        {
            /* Generate a random number and calculate backoff value (in milliseconds) for
             * the next connection retry.
             * Note: It is recommended to seed the random number generator with a device-specific
             * entropy source so that possibility of multiple devices retrying failed network operations
             * at similar intervals can be avoided. */
            xBackoffAlgStatus = BackoffAlgorithm_GetNextBackoff(&xReconnectParams, configRAND32(), &usNextRetryBackOff);

            if (xBackoffAlgStatus == BackoffAlgorithmRetriesExhausted)
            {
                LogError(("Connection to the IoT Hub failed, all attempts exhausted."));
            }
            else if (xBackoffAlgStatus == BackoffAlgorithmSuccess)
            {
                LogWarn(("Connection to the IoT Hub failed [%d]. "
                         "Retrying connection with backoff and jitter [%d]ms.",
                         xNetworkStatus, usNextRetryBackOff));
                vTaskDelay(pdMS_TO_TICKS(usNextRetryBackOff));
            }
        }
    } while ((xNetworkStatus != eTLSTransportSuccess) && (xBackoffAlgStatus == BackoffAlgorithmSuccess));

    return xNetworkStatus == eTLSTransportSuccess ? 0 : 1;
}
/*-----------------------------------------------------------*/

/*
 * @brief Create the task that demonstrates the AzureIoTHub demo
 */
void vStartDemoTask(void)
{
    /* This example uses a single application task, which in turn is used to
     * connect, subscribe, publish, unsubscribe and disconnect from the IoT Hub */
    xTaskCreate(prvAzureDemoTask,         /* Function that implements the task. */
                "AzureDemoTask",          /* Text name for the task - only used for debugging. */
                democonfigDEMO_STACKSIZE, /* Size of stack (in words, not bytes) to allocate for the task. */
                NULL,                     /* Task parameter - not used in this case. */
                tskIDLE_PRIORITY,         /* Task priority, must be between 0 and configMAX_PRIORITIES - 1. */
                NULL);                    /* Used to pass out a handle to the created task - not used in this case. */
}
/*-----------------------------------------------------------*/
