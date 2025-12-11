#pragma once

#include "driver/rmt_tx.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define RMT_RESOLUTION_HZ 10000000 // 10MHz resolution, 1 tick = 0.1us (led strip needs a high resolution)
#define RMT_GPIO_NUM      48

#define LED_NUMBERS         1
#define CHASE_SPEED_MS      10

#define WHITE 0xFFFFFF

void bsp_rmt_init(void);
void bsp_rmt_send_data(uint8_t *data, size_t size);
void bsp_rmt_task(void *arg);
void led_strip_hsv2rgb(uint32_t h, uint32_t s, uint32_t v, uint32_t *r, uint32_t *g, uint32_t *b);