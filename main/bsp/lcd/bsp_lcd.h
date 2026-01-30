#pragma once

#include "esp_types.h"
#include "lvgl.h"

#define LCD_PIN_NUM_SCLK            7
#define LCD_PIN_NUM_MOSI            6
#define LCD_PIN_NUM_DC              5
#define LCD_PIN_NUM_RST             4
#define LCD_PIN_NUM_CS              8

#define LCD_WIDTH                   240
#define LCD_HEIGHT                  320

#define LCD_PIN_NUM_TOUCH_INT     9
#define LCD_PIN_NUM_TOUCH_SDA     10
#define LCD_PIN_NUM_TOUCH_RST     15
#define LCD_PIN_NUM_TOUCH_SCL     16

#define LCD_HOST                    SPI2_HOST

// typedef enum {
//     LV_DISPLAY_ROTATION_0 = 0,
//     LV_DISPLAY_ROTATION_90,
//     LV_DISPLAY_ROTATION_180,
//     LV_DISPLAY_ROTATION_270
// } lv_display_rotation_t;

void bsp_lcd_display_init();
// void bsp_display_task(void *arg);
void bsp_lcd_set_color(uint16_t color);
void bsp_lcd_draw_image(int x, int y, int width, int height, const uint16_t *image_data);
void bsp_lcd_touch_init(void);
/** 将触摸绑定到 LVGL 输入设备，需在创建 lv_display 后调用；disp 可为 NULL 表示默认 display */
lv_indev_t *bsp_lcd_register_touch_indev(lv_display_t *disp);
void bsp_lcd_set_rotation(lv_display_rotation_t rotation);
