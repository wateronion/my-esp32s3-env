#include "ulvgl_wifi.h"
#include "bsp/wifi/bsp_wifi.h"
#include "esp_lv_adapter.h"
#include "esp_log.h"
#include <string.h>

#define ULVGL_WIFI_LIST_MAX 30

static lv_obj_t *s_wifi_list = NULL;
static lv_obj_t *s_scan_btn = NULL;
static lv_obj_t *s_status_label = NULL;
static lv_obj_t *s_keyboard = NULL;
static lv_obj_t *s_pwd_ta = NULL;
static lv_obj_t *s_connect_btn = NULL;
static lv_obj_t *s_popup = NULL;
static char s_selected_ssid[33] = {0};
static bsp_wifi_ap_info_t s_ap_list[ULVGL_WIFI_LIST_MAX];
static uint16_t s_ap_count = 0;

static void wifi_scan_cb(bsp_wifi_ap_info_t *ap_list, uint16_t ap_count)
{
    s_ap_count = (ap_count > ULVGL_WIFI_LIST_MAX) ? ULVGL_WIFI_LIST_MAX : ap_count;
    if (s_ap_count > 0 && ap_list != NULL)
    {
        memcpy(s_ap_list, ap_list, s_ap_count * sizeof(bsp_wifi_ap_info_t));
    }
}

static void connect_btn_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED && s_pwd_ta != NULL)
    {
        const char *pwd = lv_textarea_get_text(s_pwd_ta);
        ESP_LOGI("ULVGL_WIFI", "Connecting to %s...", s_selected_ssid);
        bsp_wifi_connect(s_selected_ssid, pwd);

        if (s_popup != NULL)
        {
            lv_obj_delete(s_popup);
            s_popup = NULL;
            s_keyboard = NULL;
            s_pwd_ta = NULL;
        }

        if (s_status_label != NULL)
        {
            lv_label_set_text_fmt(s_status_label, "Connecting to %s...", s_selected_ssid);
        }
    }
}

static void close_popup_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED && s_popup != NULL)
    {
        lv_obj_delete(s_popup);
        s_popup = NULL;
        s_keyboard = NULL;
        s_pwd_ta = NULL;
    }
}

static void wifi_item_click_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code != LV_EVENT_CLICKED) return;

    lv_obj_t *btn = lv_event_get_target(e);
    const char *ssid = lv_obj_get_user_data(btn);
    if (ssid == NULL) return;

    strncpy(s_selected_ssid, ssid, 32);
    s_selected_ssid[32] = '\0';

    // show popup with keyboard
    s_popup = lv_obj_create(lv_layer_top());
    lv_obj_set_size(s_popup, 220, 280);
    lv_obj_center(s_popup);
    lv_obj_set_style_border_width(s_popup, 2, 0);
    lv_obj_set_style_border_color(s_popup, lv_color_hex(0x2196F3), 0);

    lv_obj_t *title = lv_label_create(s_popup);
    lv_label_set_text_fmt(title, "Password for %s", s_selected_ssid);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 5);

    s_pwd_ta = lv_textarea_create(s_popup);
    lv_textarea_set_one_line(s_pwd_ta, true);
    lv_textarea_set_password_mode(s_pwd_ta, false);
    lv_textarea_set_placeholder_text(s_pwd_ta, "Enter password...");
    lv_obj_set_width(s_pwd_ta, 200);
    lv_obj_align(s_pwd_ta, LV_ALIGN_TOP_MID, 0, 35);

    s_keyboard = lv_keyboard_create(s_popup);
    lv_keyboard_set_textarea(s_keyboard, s_pwd_ta);
    lv_obj_align(s_keyboard, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_keyboard_set_popovers(s_keyboard, false);

    s_connect_btn = lv_button_create(s_popup);
    lv_obj_set_size(s_connect_btn, 80, 32);
    lv_obj_align(s_connect_btn, LV_ALIGN_TOP_MID, -45, 65);
    lv_obj_add_event_cb(s_connect_btn, connect_btn_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *btn_label = lv_label_create(s_connect_btn);
    lv_label_set_text(btn_label, "Connect");
    lv_obj_center(btn_label);

    lv_obj_t *cancel_btn = lv_button_create(s_popup);
    lv_obj_set_size(cancel_btn, 80, 32);
    lv_obj_align(cancel_btn, LV_ALIGN_TOP_MID, 45, 65);
    lv_obj_add_event_cb(cancel_btn, close_popup_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *cancel_label = lv_label_create(cancel_btn);
    lv_label_set_text(cancel_label, "Cancel");
    lv_obj_center(cancel_label);
}

static void update_list_ui(void)
{
    if (s_wifi_list == NULL) return;

    // clear old items (keep scroll and child objects)
    lv_obj_clean(s_wifi_list);

    if (s_ap_count == 0)
    {
        lv_obj_t *empty_label = lv_label_create(s_wifi_list);
        lv_label_set_text(empty_label, "No networks found.\nTap Scan to search.");
        lv_obj_align(empty_label, LV_ALIGN_CENTER, 0, 0);
        if (s_status_label) lv_label_set_text(s_status_label, "No networks found");
        return;
    }

    for (int i = 0; i < s_ap_count; i++)
    {
        lv_obj_t *item = lv_button_create(s_wifi_list);
        lv_obj_set_width(item, lv_pct(95));
        lv_obj_set_height(item, 36);
        lv_obj_set_style_pad_all(item, 4, 0);
        lv_obj_add_event_cb(item, wifi_item_click_cb, LV_EVENT_CLICKED, NULL);
        lv_obj_set_user_data(item, (void *)s_ap_list[i].ssid);

        // SSID label
        lv_obj_t *ssid_label = lv_label_create(item);
        lv_label_set_text(ssid_label, s_ap_list[i].ssid);
        lv_obj_align(ssid_label, LV_ALIGN_LEFT_MID, 5, 0);

        // RSSI label
        lv_obj_t *rssi_label = lv_label_create(item);
        lv_label_set_text_fmt(rssi_label, "%d dBm", s_ap_list[i].rssi);
        lv_obj_align(rssi_label, LV_ALIGN_RIGHT_MID, -25, 0);

        // lock icon for secured networks
        if (s_ap_list[i].authmode != WIFI_AUTH_OPEN)
        {
            lv_obj_t *lock = lv_label_create(item);
            lv_label_set_text(lock, LV_SYMBOL_WIFI);
            lv_obj_align(lock, LV_ALIGN_RIGHT_MID, -5, 0);
        }
    }

    if (s_status_label)
    {
        lv_label_set_text_fmt(s_status_label, "Found %d network(s)", s_ap_count);
    }
}

static void scan_timer_cb(lv_timer_t *timer)
{
    if (s_ap_count > 0 || s_status_label == NULL)
    {
        update_list_ui();
        lv_obj_clear_state(s_scan_btn, LV_STATE_DISABLED);
        lv_timer_delete(timer);
    }
    else
    {
        // scan still in progress, keep waiting
        lv_label_set_text(s_status_label, "Scanning...");
    }
}

static void scan_btn_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    if (code != LV_EVENT_CLICKED) return;

    if (s_status_label) lv_label_set_text(s_status_label, "Scanning...");
    lv_obj_add_state(s_scan_btn, LV_STATE_DISABLED);
    s_ap_count = 0;
    update_list_ui();

    esp_lv_adapter_unlock();
    bsp_wifi_scan(wifi_scan_cb);
    esp_lv_adapter_lock(-1);

    lv_timer_create(scan_timer_cb, 500, NULL);
}

void ulvgl_wifi_create(lv_obj_t *parent)
{
    if (parent == NULL) parent = lv_screen_active();

    lv_obj_set_style_bg_color(parent, lv_color_hex(0x1a1a2e), 0);

    // title bar
    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, "WiFi Manager");
    lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 5);

    // scan button
    s_scan_btn = lv_button_create(parent);
    lv_obj_set_size(s_scan_btn, 100, 30);
    lv_obj_align(s_scan_btn, LV_ALIGN_TOP_MID, 0, 28);
    lv_obj_add_event_cb(s_scan_btn, scan_btn_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *scan_label = lv_label_create(s_scan_btn);
    lv_label_set_text(scan_label, LV_SYMBOL_REFRESH " Scan");
    lv_obj_center(scan_label);

    // status label
    s_status_label = lv_label_create(parent);
    lv_obj_set_style_text_color(s_status_label, lv_color_hex(0xAAAAAA), 0);
    lv_obj_align(s_status_label, LV_ALIGN_TOP_MID, 0, 65);

    // wifi list container
    s_wifi_list = lv_obj_create(parent);
    lv_obj_set_size(s_wifi_list, 224, 200);
    lv_obj_align(s_wifi_list, LV_ALIGN_TOP_MID, 0, 85);
    lv_obj_set_flex_flow(s_wifi_list, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(s_wifi_list, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(s_wifi_list, 2, 0);
    lv_obj_set_style_pad_all(s_wifi_list, 4, 0);
    lv_obj_set_scrollbar_mode(s_wifi_list, LV_SCROLLBAR_MODE_AUTO);
    lv_obj_set_style_border_width(s_wifi_list, 1, 0);
    lv_obj_set_style_border_color(s_wifi_list, lv_color_hex(0x333366), 0);
    lv_obj_set_style_bg_color(s_wifi_list, lv_color_hex(0x16213e), 0);

    // hint label inside list
    lv_obj_t *hint = lv_label_create(s_wifi_list);
    lv_label_set_text(hint, "Press Scan to search\nfor WiFi networks");
    lv_obj_set_style_text_color(hint, lv_color_hex(0x666688), 0);
    lv_obj_align(hint, LV_ALIGN_CENTER, 0, 0);
}
