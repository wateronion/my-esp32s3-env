#include "bsp_wifi.h"
#include "esp_log.h"
#include <string.h>

#define TAG "BSP_WIFI"

static EventGroupHandle_t s_wifi_event_group;
static esp_netif_t *s_sta_netif = NULL;
static bsp_wifi_scan_done_cb_t s_scan_cb = NULL;
static int s_retry_num = 0;
static bool s_connecting = false;

static void event_handler(void *arg, esp_event_base_t event_base,
                          int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        ESP_LOGI(TAG, "wifi started, ready to scan or connect");
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        if (s_connecting && s_retry_num < BSP_WIFI_MAX_RETRY)
        {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "retry to connect to the AP");
        }
        else if (s_connecting)
        {
            s_connecting = false;
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
        ESP_LOGI(TAG, "disconnected");
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_SCAN_DONE)
    {
        uint16_t ap_count = 0;
        ESP_ERROR_CHECK(esp_wifi_scan_get_ap_num(&ap_count));

        if (ap_count == 0)
        {
            ESP_LOGI(TAG, "no AP found");
            if (s_scan_cb) s_scan_cb(NULL, 0);
            return;
        }

        uint16_t num = (ap_count > BSP_WIFI_SCAN_MAX_AP) ? BSP_WIFI_SCAN_MAX_AP : ap_count;
        wifi_ap_record_t *ap_records = calloc(num, sizeof(wifi_ap_record_t));
        if (ap_records == NULL)
        {
            if (s_scan_cb) s_scan_cb(NULL, 0);
            return;
        }

        ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&num, ap_records));

        bsp_wifi_ap_info_t *ap_list = calloc(num, sizeof(bsp_wifi_ap_info_t));
        if (ap_list == NULL)
        {
            free(ap_records);
            if (s_scan_cb) s_scan_cb(NULL, 0);
            return;
        }

        for (int i = 0; i < num; i++)
        {
            strlcpy(ap_list[i].ssid, (const char *)ap_records[i].ssid, sizeof(ap_list[i].ssid));
            ap_list[i].rssi = ap_records[i].rssi;
            ap_list[i].authmode = ap_records[i].authmode;
        }

        free(ap_records);

        if (s_scan_cb)
        {
            s_scan_cb(ap_list, num);
            s_scan_cb = NULL;
        }
        free(ap_list);
    }
}

esp_err_t bsp_wifi_init(void)
{
    if (s_sta_netif != NULL) return ESP_OK;

    s_wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    s_sta_netif = esp_netif_create_default_wifi_sta();
    assert(s_sta_netif);

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &event_handler,
                                                        NULL,
                                                        NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &event_handler,
                                                        NULL,
                                                        NULL));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "wifi initialized");
    return ESP_OK;
}

esp_err_t bsp_wifi_scan(bsp_wifi_scan_done_cb_t callback)
{
    if (s_sta_netif == NULL)
    {
        ESP_LOGE(TAG, "wifi not initialized, call bsp_wifi_init first");
        return ESP_ERR_INVALID_STATE;
    }

    s_scan_cb = callback;

    wifi_scan_config_t scan_cfg = {
        .ssid = NULL,
        .bssid = NULL,
        .channel = 0,
        .show_hidden = false,
        .scan_type = WIFI_SCAN_TYPE_ACTIVE,
    };
    ESP_ERROR_CHECK(esp_wifi_scan_start(&scan_cfg, false));

    ESP_LOGI(TAG, "scanning...");
    return ESP_OK;
}

esp_err_t bsp_wifi_connect(const char *ssid, const char *password)
{
    if (s_sta_netif == NULL)
    {
        ESP_LOGE(TAG, "wifi not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTED_BIT | WIFI_FAIL_BIT);
    s_retry_num = 0;
    s_connecting = true;

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = "",
            .password = "",
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
        },
    };
    strlcpy((char *)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid));
    strlcpy((char *)wifi_config.sta.password, password, sizeof(wifi_config.sta.password));

    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    esp_wifi_connect();

    ESP_LOGI(TAG, "connecting to SSID:%s", ssid);
    return ESP_OK;
}

esp_err_t bsp_wifi_deinit(void)
{
    esp_wifi_stop();
    esp_wifi_deinit();
    if (s_sta_netif)
    {
        esp_netif_destroy(s_sta_netif);
        s_sta_netif = NULL;
    }
    if (s_wifi_event_group)
    {
        vEventGroupDelete(s_wifi_event_group);
        s_wifi_event_group = NULL;
    }
    ESP_LOGI(TAG, "wifi deinitialized");
    return ESP_OK;
}
