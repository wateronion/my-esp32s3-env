#pragma once

#include "driver/gptimer.h"
#include "freeRTOS/FreeRTOS.h"
#include "freeRTOS/task.h"
#include "freertos/queue.h"
#include "esp_log.h"

/**
 * @brief 定时器初始化配置
 * 
 * @param resolution_hz 定时器分辨率
 * @param alarm_count  定时器报警值
 * @return QueueHandle_t 定时器回调队列
 */
QueueHandle_t TimerInitConfig(uint32_t resolution_hz, uint64_t alarm_count);

void timer_task(void *pvParameters);
