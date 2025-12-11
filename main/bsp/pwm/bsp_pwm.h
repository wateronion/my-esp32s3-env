#pragma once

#include "driver/ledc.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define LEDC_TIMER              LEDC_TIMER_0
#define LEDC_MODE               LEDC_LOW_SPEED_MODE
#define LEDC_OUTPUT_IO          (GPIO_NUM_18)
#define LEDC_CHANNEL            LEDC_CHANNEL_0
#define LEDC_DUTY_RES           LEDC_TIMER_13_BIT
#define LEDC_DUTY               (4095) // Set duty to 50%. 4095 = 100%
#define LEDC_FREQUENCY          (5000) // Set frequency at 5 kHz

void pwm_init(void);
void pwm_task(void *pvParameters);
