#pragma once

#include "esp_types.h"
#include "lvgl.h"
#include "esp_lcd_touch.h"

#define LCD_PIN_NUM_SCLK            7
#define LCD_PIN_NUM_MOSI            6
#define LCD_PIN_NUM_DC              5
#define LCD_PIN_NUM_RST             4
#define LCD_PIN_NUM_CS              8

#define LCD_WIDTH                   240
#define LCD_HEIGHT                  320

#define LCD_PIN_NUM_TOUCH_INT     -1
#define LCD_PIN_NUM_TOUCH_RST     -1
#define LCD_PIN_NUM_TOUCH_SCL     17
#define LCD_PIN_NUM_TOUCH_SDA     18

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
void bsp_lcd_touch_init(esp_lcd_touch_handle_t *ret_touch);
void bsp_lcd_set_rotation(lv_display_rotation_t rotation);
void bsp_lcd_touch_test(void);
