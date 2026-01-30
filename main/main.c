#include <stdio.h>
#include <inttypes.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "bsp/led/bsp_led.h"
#include "bsp/uart/bsp_uart.h"
#include "bsp/timer/bsp_timer.h"
#include "bsp/pwm/bsp_pwm.h"
#include "bsp/rmt/bsp_rmt.h"
#include "bsp/lcd/bsp_lcd.h"
#include "display/logo.h"
#include "display/pic.h"

void app_main(void)
{
    esp_log_level_set("*", ESP_LOG_VERBOSE);

    xTaskCreate(uart1_rx_task, "uart1_rx_task", 4096, NULL, 10, NULL);
    // xTaskCreate(timer_task, "timer_task", 4096, NULL, 10, NULL);
    // xTaskCreate(pwm_task, "pwm_task", 4096, NULL, 10, NULL);
    // xTaskCreate(bsp_rmt_task, "bsp_rmt_task", 8192, NULL, 10, NULL);
    bsp_lcd_display_init();
    while (1)
    {
        // bsp_lcd_set_color(0xF800); // 红色
        // vTaskDelay(1000 / portTICK_PERIOD_MS);

        // bsp_lcd_set_color(0x07E0); // 绿色
        // bsp_lcd_draw_image(0,0,240,320,(uint16_t *)gImage_YUNO);
        // test_display();
        vTaskDelay(1000 / portTICK_PERIOD_MS);

        // bsp_lcd_set_color(0x001F); // 蓝色
        // vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
