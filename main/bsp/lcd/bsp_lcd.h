#pragma once

#include "esp_types.h"

#define LCD_PIN_NUM_SCLK            7
#define LCD_PIN_NUM_MOSI            6
#define LCD_PIN_NUM_DC              5
#define LCD_PIN_NUM_RST             4

#define LCD_WIDTH                   240
#define LCD_HEIGHT                  320

#define LCD_HOST                    SPI2_HOST

void bsp_lcd_init();
// void bsp_display_task(void *arg);
void bsp_lcd_set_color(uint16_t color);
void bsp_lcd_draw_image(int x, int y, int width, int height, const uint16_t *image_data);
void test_display(void);
