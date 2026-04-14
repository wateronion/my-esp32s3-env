// #include "bsp_pwm.h"

// void pwm_init(void)
// {
//     // Configure PWM timer
//     ledc_timer_config_t ledc_timer = {
//         .speed_mode = LEDC_MODE,
//         .timer_num = LEDC_TIMER,
//         .duty_resolution = LEDC_DUTY_RES,
//         .freq_hz = LEDC_FREQUENCY,
//         .clk_cfg = LEDC_AUTO_CLK,
//     };

//     ledc_timer_config(&ledc_timer);

//     // Configure PWM channel
//     ledc_channel_config_t ledc_channel = {
//         .speed_mode = LEDC_MODE,
//         .channel = LEDC_CHANNEL,
//         .timer_sel = LEDC_TIMER,
//         .intr_type = LEDC_INTR_DISABLE,
//         .gpio_num = LEDC_OUTPUT_IO,
//         .duty = 0,
//         .hpoint = 0,
//     };

//     ledc_channel_config(&ledc_channel);
// }

// void pwm_task(void *pvParameters)
// {
//     pwm_init();

//     // int duty_val = 0;
//     // ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, duty_val);
//     // ledc_update_duty(LEDC_MODE, LEDC_CHANNEL);

//     // while (1)
//     // {
//     //     for (duty_val = 0; duty_val < 8191; duty_val += 100)
//     //     {
//     //         // 设置亮度模拟值，占空比不断加大
//     //         ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, duty_val);
//     //         ledc_update_duty(LEDC_MODE, LEDC_CHANNEL);
//     //         // 延时 10ms
//     //         vTaskDelay(10 / portTICK_PERIOD_MS);
//     //     }
//     //     // 实现渐灭效果
//     //     for (duty_val = 8191; duty_val >= 0; duty_val -= 100)
//     //     {
//     //         // 设置亮度模拟值，占空比不断减少
//     //         ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, duty_val);
//     //         ledc_update_duty(LEDC_MODE, LEDC_CHANNEL);
//     //         // 延时 10ms
//     //         vTaskDelay(10 / portTICK_PERIOD_MS);
//     //     }
//     // }
// }