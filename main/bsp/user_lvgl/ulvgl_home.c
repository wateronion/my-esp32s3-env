#include "ulvgl_home.h"
#include "ulvgl_wifi.h"
#include "bsp/dht11/bsp_dht11.h"
#include "bsp/controls/bsp_controls.h"
#include "esp_log.h"

#define TAG "ULVGL_HOME"

static lv_obj_t *s_home_scr = NULL;
static lv_obj_t *s_temp_label = NULL;
static lv_obj_t *s_humid_label = NULL;
static lv_obj_t *s_ac_btn = NULL;
static lv_obj_t *s_ac_label = NULL;
static lv_obj_t *s_light_btn = NULL;
static lv_obj_t *s_light_label = NULL;
static lv_obj_t *s_curtain_label = NULL;

static void update_sensor_data(lv_timer_t *timer)
{
    int humidity = 0;
    float temperature = 0;

    if (bsp_dht11_get_cached(&humidity, &temperature) == ESP_OK)
    {
        if (s_temp_label)
            lv_label_set_text_fmt(s_temp_label, "%.1f C", temperature);
        if (s_humid_label)
            lv_label_set_text_fmt(s_humid_label, "%d%%", humidity);
    }
}

static void ac_btn_cb(lv_event_t *e)
{
    bsp_ac_toggle();
    bool on = bsp_ac_get_state();
    if (s_ac_label)
        lv_label_set_text(s_ac_label, on ? "AC: ON" : "AC: OFF");
    if (s_ac_btn)
        lv_obj_set_style_bg_color(s_ac_btn, on ? lv_color_hex(0x1565C0) : lv_color_hex(0x2196F3), 0);
}

static void light_btn_cb(lv_event_t *e)
{
    bsp_light_toggle();
    bool on = bsp_light_get_state();
    if (s_light_label)
        lv_label_set_text(s_light_label, on ? "Light: ON" : "Light: OFF");
    if (s_light_btn)
        lv_obj_set_style_bg_color(s_light_btn, on ? lv_color_hex(0xE65100) : lv_color_hex(0xFF9800), 0);
}

static void curtain_open_cb(lv_event_t *e)
{
    bsp_curtain_open();
    if (s_curtain_label)
        lv_label_set_text(s_curtain_label, "Curtain: Opened");
}

static void curtain_close_cb(lv_event_t *e)
{
    bsp_curtain_close();
    if (s_curtain_label)
        lv_label_set_text(s_curtain_label, "Curtain: Closed");
}

void ulvgl_home_go_back(void)
{
    lv_obj_t *top = lv_screen_active();
    if (top != s_home_scr)
    {
        lv_obj_delete(top);
    }
    if (s_home_scr)
    {
        lv_screen_load(s_home_scr);
    }
}

static void wifi_btn_cb(lv_event_t *e)
{
    lv_obj_t *wifi_scr = lv_obj_create(NULL);
    ulvgl_wifi_create(wifi_scr, ulvgl_home_go_back);
    lv_screen_load(wifi_scr);
}

void ulvgl_home_create(lv_obj_t *parent)
{
    s_home_scr = parent;
    if (parent == NULL) parent = lv_screen_active();

    /* White background */
    lv_obj_set_style_bg_color(parent, lv_color_hex(0xFFFFFF), 0);

    /* Title */
    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, "Smart Home");
    lv_obj_set_style_text_color(title, lv_color_hex(0x000000), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);

    /* Temperature Card */
    lv_obj_t *temp_card = lv_obj_create(parent);
    lv_obj_set_size(temp_card, 105, 80);
    lv_obj_align(temp_card, LV_ALIGN_TOP_LEFT, 10, 45);
    lv_obj_set_style_bg_color(temp_card, lv_color_hex(0xF0F8FF), 0);
    lv_obj_set_style_border_width(temp_card, 1, 0);
    lv_obj_set_style_border_color(temp_card, lv_color_hex(0xDDDDDD), 0);
    lv_obj_set_style_radius(temp_card, 8, 0);

    s_temp_label = lv_label_create(temp_card);
    lv_label_set_text(s_temp_label, "-- C");
    lv_obj_set_style_text_color(s_temp_label, lv_color_hex(0x00AA55), 0);
    lv_obj_center(s_temp_label);

    lv_obj_t *temp_sub = lv_label_create(temp_card);
    lv_label_set_text(temp_sub, "Temperature");
    lv_obj_set_style_text_color(temp_sub, lv_color_hex(0x999999), 0);
    lv_obj_align(temp_sub, LV_ALIGN_BOTTOM_MID, 0, -4);

    /* Humidity Card */
    lv_obj_t *humid_card = lv_obj_create(parent);
    lv_obj_set_size(humid_card, 105, 80);
    lv_obj_align(humid_card, LV_ALIGN_TOP_RIGHT, -10, 45);
    lv_obj_set_style_bg_color(humid_card, lv_color_hex(0xF0F8FF), 0);
    lv_obj_set_style_border_width(humid_card, 1, 0);
    lv_obj_set_style_border_color(humid_card, lv_color_hex(0xDDDDDD), 0);
    lv_obj_set_style_radius(humid_card, 8, 0);

    s_humid_label = lv_label_create(humid_card);
    lv_label_set_text(s_humid_label, "--%");
    lv_obj_set_style_text_color(s_humid_label, lv_color_hex(0x0077CC), 0);
    lv_obj_center(s_humid_label);

    lv_obj_t *humid_sub = lv_label_create(humid_card);
    lv_label_set_text(humid_sub, "Humidity");
    lv_obj_set_style_text_color(humid_sub, lv_color_hex(0x999999), 0);
    lv_obj_align(humid_sub, LV_ALIGN_BOTTOM_MID, 0, -4);

    /* AC Button */
    s_ac_btn = lv_button_create(parent);
    lv_obj_set_size(s_ac_btn, 105, 48);
    lv_obj_align(s_ac_btn, LV_ALIGN_TOP_LEFT, 10, 140);
    lv_obj_add_event_cb(s_ac_btn, ac_btn_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_set_style_bg_color(s_ac_btn, lv_color_hex(0x2196F3), 0);
    lv_obj_set_style_radius(s_ac_btn, 8, 0);

    s_ac_label = lv_label_create(s_ac_btn);
    lv_label_set_text(s_ac_label, "AC: OFF");
    lv_obj_set_style_text_color(s_ac_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_center(s_ac_label);

    /* Light Button */
    s_light_btn = lv_button_create(parent);
    lv_obj_set_size(s_light_btn, 105, 48);
    lv_obj_align(s_light_btn, LV_ALIGN_TOP_RIGHT, -10, 140);
    lv_obj_add_event_cb(s_light_btn, light_btn_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_set_style_bg_color(s_light_btn, lv_color_hex(0xFF9800), 0);
    lv_obj_set_style_radius(s_light_btn, 8, 0);

    s_light_label = lv_label_create(s_light_btn);
    lv_label_set_text(s_light_label, "Light: OFF");
    lv_obj_set_style_text_color(s_light_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_center(s_light_label);

    /* Curtain Status */
    s_curtain_label = lv_label_create(parent);
    lv_label_set_text(s_curtain_label, "Curtain: Closed");
    lv_obj_set_style_text_color(s_curtain_label, lv_color_hex(0x666666), 0);
    lv_obj_align(s_curtain_label, LV_ALIGN_TOP_LEFT, 10, 205);

    /* Curtain Open Button */
    lv_obj_t *open_btn = lv_button_create(parent);
    lv_obj_set_size(open_btn, 100, 36);
    lv_obj_align(open_btn, LV_ALIGN_TOP_LEFT, 10, 235);
    lv_obj_add_event_cb(open_btn, curtain_open_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_set_style_bg_color(open_btn, lv_color_hex(0x4CAF50), 0);
    lv_obj_set_style_radius(open_btn, 6, 0);
    lv_obj_t *open_lbl = lv_label_create(open_btn);
    lv_label_set_text(open_lbl, "Open");
    lv_obj_set_style_text_color(open_lbl, lv_color_hex(0xFFFFFF), 0);
    lv_obj_center(open_lbl);

    /* Curtain Close Button */
    lv_obj_t *close_btn = lv_button_create(parent);
    lv_obj_set_size(close_btn, 100, 36);
    lv_obj_align(close_btn, LV_ALIGN_TOP_RIGHT, -10, 235);
    lv_obj_add_event_cb(close_btn, curtain_close_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_set_style_bg_color(close_btn, lv_color_hex(0xF44336), 0);
    lv_obj_set_style_radius(close_btn, 6, 0);
    lv_obj_t *close_lbl = lv_label_create(close_btn);
    lv_label_set_text(close_lbl, "Close");
    lv_obj_set_style_text_color(close_lbl, lv_color_hex(0xFFFFFF), 0);
    lv_obj_center(close_lbl);

    /* WiFi Settings Button */
    lv_obj_t *wifi_btn = lv_button_create(parent);
    lv_obj_set_size(wifi_btn, 220, 36);
    lv_obj_align(wifi_btn, LV_ALIGN_BOTTOM_MID, 0, -10);
    lv_obj_add_event_cb(wifi_btn, wifi_btn_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_set_style_bg_color(wifi_btn, lv_color_hex(0x533483), 0);
    lv_obj_set_style_radius(wifi_btn, 6, 0);
    lv_obj_t *wifi_lbl = lv_label_create(wifi_btn);
    lv_label_set_text(wifi_lbl, LV_SYMBOL_WIFI " WiFi Settings");
    lv_obj_set_style_text_color(wifi_lbl, lv_color_hex(0xFFFFFF), 0);
    lv_obj_center(wifi_lbl);

    /* Timer: update sensor data every 10 seconds */
    lv_timer_create(update_sensor_data, 10000, NULL);
    update_sensor_data(NULL);

    ESP_LOGI(TAG, "Home page created");
}
