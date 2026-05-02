#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"

#define DHT11_GPIO      3
#define DHT11_TASK_PRIO 5
#define DHT11_TASK_STACK 4096

esp_err_t bsp_dht11_init(void);
esp_err_t bsp_dht11_read(int *humidity, float *temperature);
esp_err_t bsp_dht11_get_cached(int *humidity, float *temperature);
void      bsp_dht11_task(void *arg);
