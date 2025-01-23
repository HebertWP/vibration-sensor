#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sys.h"


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
DynamicJsonDocument deviceDocument(1024);
char certificate[2048];
char key[2048];
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
    FILE* f = fopen("/spiffs/device_config.json", "r");
    char line[128];    

    fgets(line, sizeof(line), f);
    deserializeJson(deviceDocument, line);
    fclose(f);
    const char *host = (const char*)deviceDocument["host"];
    const char *device_id = (const char*)deviceDocument["device_id"];
    az_iot_hub_client_options options = az_iot_hub_client_options_default();
    options.user_agent = AZ_SPAN_FROM_STR(AZURE_SDK_CLIENT_USER_AGENT);

    if (az_result_failed(az_iot_hub_client_init(
            &client,
            az_span_create((uint8_t *)host, strlen(host)),
            az_span_create((uint8_t *)device_id, strlen(device_id)),
            &options)))
    {
        ESP_LOGI("IoT Hub Client", "Failed to initialize IoT Hub client");
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
    esp_mqtt_client_config_t mqtt_config;
    memset(&mqtt_config, 0, sizeof(mqtt_config));

    std::string uri = std::string("mqtts://") + (const char *)deviceDocument["host"];
    mqtt_config.broker.address.uri = uri.c_str();
    mqtt_config.broker.address.port = mqtt_port;
    mqtt_config.credentials.client_id = mqtt_client_id;
    mqtt_config.credentials.username = mqtt_username;

    FILE* f = fopen("/spiffs/ca.pem", "r");
    char line[256];
    int index = 0;
    while(fgets(line, sizeof(line), f))
    {
        for(int i = 0; i < strlen(line); i++)
        {
            certificate[index++] = line[i];
        }
    }
    certificate[index -1] = '\0';
    fclose(f);
    mqtt_config.credentials.authentication.certificate = certificate;
    mqtt_config.credentials.authentication.certificate_len = index;
    
    f = fopen("/spiffs/cert_key.key", "r");
    index = 0;
    while(fgets(line, sizeof(line), f))
    {
        for(int i = 0; i < strlen(line); i++)
        {
            key[index++] = line[i];
        }
    }
    key[index -1] = '\0';

    mqtt_config.credentials.authentication.key = key;
    mqtt_config.credentials.authentication.key_len = index;

    mqtt_config.session.keepalive = 30;
    mqtt_config.session.disable_clean_session = 0;
    mqtt_config.network.disable_auto_reconnect = false;
    mqtt_config.broker.verification.certificate = (const char *)ca_pem;
    mqtt_config.broker.verification.certificate_len = (size_t)ca_pem_len;

    mqtt_client = esp_mqtt_client_init(&mqtt_config);

    if (mqtt_client == NULL)
    {
        ESP_LOGI("MQTT", "Failed to initialize MQTT client");
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
    ESP_LOGI("Telemetry", "BEFORE xSemaphoreTake");
    xSemaphoreTake(canRead, portMAX_DELAY);
    ESP_LOGI("Telemetry", "AFTER xSemaphoreTake");
    JsonDocument doc;
    for (int i = 0; i < 128; i++)
    {
        doc["data"][i] = reads[i];
    }

    doc["timestamp"] = getEpochTimeInSecs();
    serializeJson(doc, telemetry_payload);
    xSemaphoreGive(canRead);
}

static void sendTelemetry()
{
    if (az_result_failed(az_iot_hub_client_telemetry_get_publish_topic(
            &client, NULL, telemetry_topic, sizeof(telemetry_topic), NULL)))
    {
        return;
    }
    ESP_LOGI("Telemetry", "Sending telemetry message");
    generateTelemetryPayload();
    
    ESP_LOGI("Telemetry", "Telemetry payload: %s", telemetry_payload.c_str());
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
        sendTelemetry();
        ESP_LOGI("Telemetry", "Telemetry sent");
        delay(45000);
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