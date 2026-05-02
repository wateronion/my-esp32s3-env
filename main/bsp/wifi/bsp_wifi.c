#include "bsp_wifi.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include <string.h>

#define TAG "BSP_WIFI"
#define NVS_NAMESPACE "wifi_creds"
#define NVS_KEY_SSID   "ssid"
#define NVS_KEY_PASS   "password"

static EventGroupHandle_t s_wifi_event_group;
static esp_netif_t *s_sta_netif = NULL;
static bsp_wifi_scan_done_cb_t s_scan_cb = NULL;
static int s_retry_num = 0;
static bool s_connecting = false;
static bool s_connected = false;
static char s_saved_ssid[33] = {0};
static char s_saved_password[65] = {0};

static esp_err_t save_creds_to_nvs(const char *ssid, const char *password)
{
    nvs_handle_t handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle);
    if (err != ESP_OK) return err;

    err = nvs_set_str(handle, NVS_KEY_SSID, ssid);
    if (err == ESP_OK)
        err = nvs_set_str(handle, NVS_KEY_PASS, password);
    if (err == ESP_OK)
        err = nvs_commit(handle);

    nvs_close(handle);
    return err;
}

static esp_err_t load_creds_from_nvs(char *ssid, size_t ssid_size,
                                     char *password, size_t pass_size)
{
    nvs_handle_t handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READONLY, &handle);
    if (err != ESP_OK) return err;

    size_t len = ssid_size;
    err = nvs_get_str(handle, NVS_KEY_SSID, ssid, &len);
    if (err != ESP_OK) {
        nvs_close(handle);
        return err;
    }

    len = pass_size;
    err = nvs_get_str(handle, NVS_KEY_PASS, password, &len);

    nvs_close(handle);
    return err;
}

static void clear_creds_from_nvs(void)
{
    nvs_handle_t handle;
    if (nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle) != ESP_OK) return;

    nvs_erase_key(handle, NVS_KEY_SSID);
    nvs_erase_key(handle, NVS_KEY_PASS);
    nvs_commit(handle);
    nvs_close(handle);
}

static void event_handler(void *arg, esp_event_base_t event_base,
                          int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        ESP_LOGI(TAG, "wifi started, ready to scan or connect");
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        s_connected = false;
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
        s_connecting = false;
        s_connected = true;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);

        // save credentials on successful connection
        if (strlen(s_saved_ssid) > 0)
        {
            esp_err_t err = save_creds_to_nvs(s_saved_ssid, s_saved_password);
            if (err == ESP_OK)
                ESP_LOGI(TAG, "credentials saved to NVS");
            else
                ESP_LOGW(TAG, "failed to save credentials: %d", err);
        }
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

    // check NVS for saved credentials and auto-reconnect
    char saved_ssid[33] = {0};
    char saved_pass[65] = {0};
    if (load_creds_from_nvs(saved_ssid, sizeof(saved_ssid),
                            saved_pass, sizeof(saved_pass)) == ESP_OK)
    {
        strlcpy(s_saved_ssid, saved_ssid, sizeof(s_saved_ssid));
        strlcpy(s_saved_password, saved_pass, sizeof(s_saved_password));

        wifi_config_t wifi_cfg = {0};
        strlcpy((char *)wifi_cfg.sta.ssid, saved_ssid, sizeof(wifi_cfg.sta.ssid));
        strlcpy((char *)wifi_cfg.sta.password, saved_pass, sizeof(wifi_cfg.sta.password));
        wifi_cfg.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
        ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_cfg));

        s_retry_num = 0;
        s_connecting = true;
        esp_wifi_connect();
        ESP_LOGI(TAG, "auto-reconnecting to saved SSID:%s", saved_ssid);
    }

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

    // stop any ongoing connection so scan is allowed
    if (s_connecting || s_connected)
    {
        esp_wifi_disconnect();
        s_connecting = false;
        s_connected = false;
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    s_scan_cb = callback;

    wifi_scan_config_t scan_cfg = {
        .ssid = NULL,
        .bssid = NULL,
        .channel = 0,
        .show_hidden = false,
        .scan_type = WIFI_SCAN_TYPE_ACTIVE,
    };
    esp_err_t ret = esp_wifi_scan_start(&scan_cfg, false);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "scan start failed: %d", ret);
        return ret;
    }

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
    s_connected = false;
    strlcpy(s_saved_ssid, ssid, sizeof(s_saved_ssid));
    strlcpy(s_saved_password, password, sizeof(s_saved_password));

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

esp_err_t bsp_wifi_disconnect(void)
{
    esp_wifi_disconnect();
    s_connecting = false;
    s_connected = false;
    s_retry_num = 0;
    ESP_LOGI(TAG, "disconnected, saved credentials kept for reconnect");
    return ESP_OK;
}

esp_err_t bsp_wifi_reconnect(void)
{
    if (s_sta_netif == NULL)
    {
        ESP_LOGE(TAG, "wifi not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    char saved_ssid[33] = {0};
    char saved_pass[65] = {0};
    if (load_creds_from_nvs(saved_ssid, sizeof(saved_ssid),
                            saved_pass, sizeof(saved_pass)) != ESP_OK)
    {
        ESP_LOGE(TAG, "no saved credentials");
        return ESP_ERR_INVALID_STATE;
    }

    // also update static buffers
    strlcpy(s_saved_ssid, saved_ssid, sizeof(s_saved_ssid));
    strlcpy(s_saved_password, saved_pass, sizeof(s_saved_password));

    xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTED_BIT | WIFI_FAIL_BIT);
    s_retry_num = 0;
    s_connecting = true;
    s_connected = false;

    wifi_config_t wifi_cfg = {0};
    strlcpy((char *)wifi_cfg.sta.ssid, saved_ssid, sizeof(wifi_cfg.sta.ssid));
    strlcpy((char *)wifi_cfg.sta.password, saved_pass, sizeof(wifi_cfg.sta.password));
    wifi_cfg.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_cfg));
    esp_wifi_connect();
    ESP_LOGI(TAG, "reconnecting to saved SSID:%s", saved_ssid);
    return ESP_OK;
}

bool bsp_wifi_is_connected(void)
{
    return s_connected;
}

bool bsp_wifi_has_saved(void)
{
    nvs_handle_t handle;
    if (nvs_open(NVS_NAMESPACE, NVS_READONLY, &handle) != ESP_OK)
        return false;

    char ssid[33] = {0};
    size_t len = sizeof(ssid);
    esp_err_t err = nvs_get_str(handle, NVS_KEY_SSID, ssid, &len);
    nvs_close(handle);
    return (err == ESP_OK && strlen(ssid) > 0);
}

int bsp_wifi_get_saved_ssid(char *ssid, size_t size)
{
    if (size == 0) return 0;
    ssid[0] = '\0';

    nvs_handle_t handle;
    if (nvs_open(NVS_NAMESPACE, NVS_READONLY, &handle) != ESP_OK)
        return 0;

    size_t len = size;
    esp_err_t err = nvs_get_str(handle, NVS_KEY_SSID, ssid, &len);
    nvs_close(handle);

    if (err != ESP_OK) return 0;
    return strlen(ssid);
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
