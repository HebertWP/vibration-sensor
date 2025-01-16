#include "iot_setup.h"
// C99 libraries
#include <cstdlib>
#include <string.h>
#include <time.h>

// Libraries for MQTT client and WiFi connection
#include <WiFi.h>
#include <mqtt_client.h>

// Azure IoT SDK for C includes
#include <az_core.h>
#include <az_iot.h>
#include <azure_ca.h>

// Additional sample headers
#include "AzIoTSasToken.h"
#include "SerialLogger.h"
#include "iot_configs.h"
#include "QMI8658.h"
#include <ArduinoJson.h>

// When developing for your own Arduino-based platform,
// please follow the format '(ard;<platform>)'.
#define AZURE_SDK_CLIENT_USER_AGENT "c%2F" AZ_SDK_VERSION_STRING "(ard;esp32)"

// Utility macros and defines
#define sizeofarray(a) (sizeof(a) / sizeof(a[0]))
#define NTP_SERVERS "pool.ntp.org", "time.nist.gov"
#define MQTT_QOS1 1
#define DO_NOT_RETAIN_MSG 0
#define SAS_TOKEN_DURATION_IN_MINUTES 60
#define UNIX_TIME_NOV_13_2017 1510592825

#define PST_TIME_ZONE -8
#define PST_TIME_ZONE_DAYLIGHT_SAVINGS_DIFF 1

#define GMT_OFFSET_SECS (PST_TIME_ZONE * 3600)
#define GMT_OFFSET_SECS_DST ((PST_TIME_ZONE + PST_TIME_ZONE_DAYLIGHT_SAVINGS_DIFF) * 3600)

// Translate iot_configs.h defines into variables used by the sample
static const char *ssid = IOT_CONFIG_WIFI_SSID;
static const char *password = IOT_CONFIG_WIFI_PASSWORD;
static const char *host = IOT_CONFIG_IOTHUB_FQDN;
static const char *mqtt_broker_uri = "mqtts://" IOT_CONFIG_IOTHUB_FQDN;
static const char *device_id = IOT_CONFIG_DEVICE_ID;
static const int mqtt_port = AZ_IOT_DEFAULT_MQTT_CONNECT_PORT;

// Memory allocated for the sample's variables and structures.
static esp_mqtt_client_handle_t mqtt_client;
static az_iot_hub_client client;

static char mqtt_client_id[128];
static char mqtt_username[128];
static char mqtt_password[200];
static uint8_t sas_signature_buffer[256];
static char telemetry_topic[128];
static uint32_t telemetry_send_count = 0;
static String telemetry_payload = "{}";

#define INCOMING_DATA_BUFFER_SIZE 128
static char incoming_data[INCOMING_DATA_BUFFER_SIZE];

// Auxiliary functions
#ifndef IOT_CONFIG_USE_X509_CERT
static AzIoTSasToken sasToken(
    &client,
    AZ_SPAN_FROM_STR(IOT_CONFIG_DEVICE_KEY),
    AZ_SPAN_FROM_BUFFER(sas_signature_buffer),
    AZ_SPAN_FROM_BUFFER(mqtt_password));
#endif // IOT_CONFIG_USE_X509_CERT

static void connectToWiFi()
{
    ESP_LOGI("Wifi", "Connecting to WiFi");

    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(100);
    WiFi.begin(ssid, password);
    WiFi.setAutoReconnect(true);
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
    }
    ESP_LOGI("Wifi", "Connected to WiFi");
}

static void initializeTime()
{
    ESP_LOGI("Time", "Initializing time");
    configTime(GMT_OFFSET_SECS, GMT_OFFSET_SECS_DST, NTP_SERVERS);
    time_t now = time(NULL);
    while (now < UNIX_TIME_NOV_13_2017)
    {
        delay(500);
        now = time(nullptr);
    }
    ESP_LOGI("Time", "Time initialized");
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    (void)handler_args;
    (void)base;
    (void)event_id;

    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;
    switch (event->event_id)
    {
        int i, r;
    case MQTT_EVENT_CONNECTED:
        r = esp_mqtt_client_subscribe(mqtt_client, AZ_IOT_HUB_CLIENT_C2D_SUBSCRIBE_TOPIC, 1);
        break;
    case MQTT_EVENT_DATA:
        
        for (i = 0; i < (INCOMING_DATA_BUFFER_SIZE - 1) && i < event->topic_len; i++)
        {
            incoming_data[i] = event->topic[i];
        }
        incoming_data[i] = '\0';

        for (i = 0; i < (INCOMING_DATA_BUFFER_SIZE - 1) && i < event->data_len; i++)
        {
            incoming_data[i] = event->data[i];
        }
        incoming_data[i] = '\0';

        break;
    default:
        break;
    }
}

static void initializeIoTHubClient()
{
    az_iot_hub_client_options options = az_iot_hub_client_options_default();
    options.user_agent = AZ_SPAN_FROM_STR(AZURE_SDK_CLIENT_USER_AGENT);

    if (az_result_failed(az_iot_hub_client_init(
            &client,
            az_span_create((uint8_t *)host, strlen(host)),
            az_span_create((uint8_t *)device_id, strlen(device_id)),
            &options)))
    {
        return;
    }

    size_t client_id_length;
    if (az_result_failed(az_iot_hub_client_get_client_id(
            &client, mqtt_client_id, sizeof(mqtt_client_id) - 1, &client_id_length)))
    {
        return;
    }

    if (az_result_failed(az_iot_hub_client_get_user_name(
            &client, mqtt_username, sizeofarray(mqtt_username), NULL)))
    {
        return;
    }
}

static int initializeMqttClient()
{
    if (sasToken.Generate(SAS_TOKEN_DURATION_IN_MINUTES) != 0)
    {
        return 1;
    }
    esp_mqtt_client_config_t mqtt_config;
    memset(&mqtt_config, 0, sizeof(mqtt_config));

    mqtt_config.broker.address.uri = mqtt_broker_uri;
    mqtt_config.broker.address.port = mqtt_port;
    mqtt_config.credentials.client_id = mqtt_client_id;
    mqtt_config.credentials.username = mqtt_username;

    mqtt_config.credentials.authentication.password = (const char *)az_span_ptr(sasToken.Get());

    mqtt_config.session.keepalive = 30;
    mqtt_config.session.disable_clean_session = 0;
    mqtt_config.network.disable_auto_reconnect = false;
    mqtt_config.broker.verification.certificate = (const char *)ca_pem;
    mqtt_config.broker.verification.certificate_len = (size_t)ca_pem_len;

    mqtt_client = esp_mqtt_client_init(&mqtt_config);

    if (mqtt_client == NULL)
    {
        return 1;
    }

    esp_mqtt_client_register_event(mqtt_client, MQTT_EVENT_ANY, mqtt_event_handler, NULL);

    esp_err_t start_result = esp_mqtt_client_start(mqtt_client);

    if (start_result != ESP_OK)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

static uint32_t getEpochTimeInSecs() { return (uint32_t)time(NULL); }

static void generateTelemetryPayload()
{
    xSemaphoreTake(xMutex, portMAX_DELAY);
    JsonDocument doc;
    for (int i = 0; i < BUFFER_SIZE; i++)
    {
        doc["data"][i] = vReal[i];
    }

    doc["timestamp"] = getEpochTimeInSecs();
    serializeJson(doc, telemetry_payload);
    Serial.println(telemetry_payload);
    xSemaphoreGive(xMutex);
}

static void sendTelemetry()
{
    if (az_result_failed(az_iot_hub_client_telemetry_get_publish_topic(
            &client, NULL, telemetry_topic, sizeof(telemetry_topic), NULL)))
    {
        return;
    }

    generateTelemetryPayload();

    esp_mqtt_client_publish(
            mqtt_client,
            telemetry_topic,
            (const char *)telemetry_payload.c_str(),
            telemetry_payload.length(),
            MQTT_QOS1,
            DO_NOT_RETAIN_MSG);
}

void vTaskSendTelemetry(void *pvParameters)
{
    while (true)
    {
        if (sasToken.IsExpired())
        {
            (void)esp_mqtt_client_destroy(mqtt_client);
            initializeMqttClient();
        }
        sendTelemetry();
        delay(60000);
    }
}

extern void establishConnection()
{
    
    connectToWiFi();
    initializeTime();
    initializeIoTHubClient();
    initializeMqttClient();
    xTaskCreate(vTaskSendTelemetry, "SendTelemetry", 20024, NULL, 1, NULL);
}