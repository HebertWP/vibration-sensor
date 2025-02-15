#ifndef __WIFI_SETUP_H__
#define __WIFI_SETUP_H__

#define SNTP_SERVER_FQDN "pool.ntp.org"

esp_err_t init_wifi();

uint64_t ullGetUnixTime(void);

bool xAzureSample_IsConnectedToInternet();

#endif // IOT_SETUP_H