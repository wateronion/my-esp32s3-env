#pragma once

#include "esp_wifi.h"
#include "esp_event.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

#define BSP_WIFI_MAX_RETRY      5
#define BSP_WIFI_SCAN_MAX_AP    30

typedef struct {
    char ssid[33];
    int8_t rssi;
    wifi_auth_mode_t authmode;
} bsp_wifi_ap_info_t;

typedef void (*bsp_wifi_scan_done_cb_t)(bsp_wifi_ap_info_t *ap_list, uint16_t ap_count);

esp_err_t bsp_wifi_init(void);
esp_err_t bsp_wifi_scan(bsp_wifi_scan_done_cb_t callback);
esp_err_t bsp_wifi_connect(const char *ssid, const char *password);
esp_err_t bsp_wifi_deinit(void);
