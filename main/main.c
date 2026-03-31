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
#include "esp_lv_adapter.h"
#include "bsp/user_lvgl/ulvgl.h"

void app_main(void)
{
    esp_log_level_set("*", ESP_LOG_VERBOSE);

    xTaskCreate(uart1_rx_task, "uart1_rx_task", 4096, NULL, 10, NULL);
    // xTaskCreate(timer_task, "timer_task", 4096, NULL, 10, NULL);
    // xTaskCreate(pwm_task, "pwm_task", 4096, NULL, 10, NULL);
    xTaskCreate(bsp_rmt_task, "bsp_rmt_task", 8192, NULL, 10, NULL);
    bsp_lcd_display_init();

    if (esp_lv_adapter_lock(-1) == ESP_OK)
    {
        lv_obj_t * parent = lv_obj_create(lv_screen_active());  /* 在当前屏幕上创建一个父级 Widget */
        lv_obj_set_pos(parent, 30, 100);
        lv_obj_set_size(parent, 100, 100);                       /* 设置父级的尺寸 */
        lv_obj_set_style_bg_color(parent, lv_color_hex(0x003a57), LV_PART_MAIN);

        lv_obj_t * obj1 = lv_obj_create(parent);                /* 在先前创建的父级 Widget 上创建一个 Widget */
        lv_obj_set_pos(obj1, 10, 10);                        /* 设置新 Widget 的位置 */
        lv_obj_set_size(obj1, 50, 50);
        lv_obj_set_style_bg_color(obj1, lv_color_hex(0xFF0000), LV_PART_MAIN);

        esp_lv_adapter_unlock();
    }

    while (1)
    {
        // bsp_lcd_set_color(0xF800); // 红色
        // vTaskDelay(1000 / portTICK_PERIOD_MS);

        // bsp_lcd_set_color(0x07E0); // 绿色
        // bsp_lcd_draw_image(0,0,240,320,(uint16_t *)gImage_YUNO);
        // test_display();
        // bsp_lcd_touch_test();
        
        vTaskDelay(100 / portTICK_PERIOD_MS);

        // bsp_lcd_set_color(0x001F); // 蓝色
        // vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}
