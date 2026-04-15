#include "bsp_rmt.h"

#define TAG "BSP_RMT"

rmt_channel_handle_t tx_channel = NULL;
rmt_encoder_handle_t rmt_encoder = NULL;
rmt_transmit_config_t tx_config_params = {0};

// static uint8_t led_strip_pixels[LED_NUMBERS * 3];

void bsp_rmt_init(void)
{
    ESP_LOGI(TAG, "Creat RMT TX channel");
    rmt_tx_channel_config_t tx_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .gpio_num = RMT_GPIO_NUM, 
        .mem_block_symbols = 64, // 内存块大小，64 * 4 bytes
        .resolution_hz = RMT_RESOLUTION_HZ, // 1 MHz 分辨率
        .trans_queue_depth = 4, // 传输队列深度
        .flags.invert_out = 0, // 输出信号是否反转
        .flags.with_dma = 0,   // 是否使用 DMA
    };
    ESP_ERROR_CHECK(rmt_new_tx_channel(&tx_config, &tx_channel));

    ESP_LOGI(TAG, "Install encoder");
    rmt_bytes_encoder_config_t rmt_bytes_encoder = {
        .bit0 = {
            .level0 = 1,
            .duration0 = 0.3 * (RMT_RESOLUTION_HZ / 1000000), // 0.3us T0H
            .level1 = 0,
            .duration1 = 0.6 * (RMT_RESOLUTION_HZ / 1000000), // 0.6us T0L
        },
        .bit1 = {
            .level0 = 1,
            .duration0 = 0.6 * (RMT_RESOLUTION_HZ / 1000000), // 0.6us T1H
            .level1 = 0,
            .duration1 = 0.3 * (RMT_RESOLUTION_HZ / 1000000), // 0.3us T1L
        },
        .flags.msb_first = true, // 从高位开始编码
    };

    ESP_ERROR_CHECK(rmt_new_bytes_encoder(&rmt_bytes_encoder, &rmt_encoder));

    ESP_LOGI(TAG, "RMT init success");

    tx_config_params.loop_count = 0; // 不循环发送

    ESP_ERROR_CHECK(rmt_enable(tx_channel));
}

void bsp_rmt_send_data(uint8_t *data, size_t size)
{
    ESP_ERROR_CHECK(rmt_transmit(tx_channel, rmt_encoder, data, size, &tx_config_params));
}

void bsp_rmt_set_color(uint32_t color)
{
    uint8_t temp[3];
    temp[0] = (color >> 16) & 0xFF; //
    temp[1] = (color >> 8) & 0xFF;
    temp[2] = color & 0xFF;
    bsp_rmt_send_data(temp, 3);
}

void led_strip_hsv2rgb(uint32_t h, uint32_t s, uint32_t v, uint32_t *r, uint32_t *g, uint32_t *b)
{
    h %= 360; // h -> [0,360]
    uint32_t rgb_max = v * 2.55f;
    uint32_t rgb_min = rgb_max * (100 - s) / 100.0f;

    uint32_t i = h / 60;
    uint32_t diff = h % 60;

    // RGB adjustment amount by hue
    uint32_t rgb_adj = (rgb_max - rgb_min) * diff / 60;

    switch (i) {
    case 0:
        *r = rgb_max;
        *g = rgb_min + rgb_adj;
        *b = rgb_min;
        break;
    case 1:
        *r = rgb_max - rgb_adj;
        *g = rgb_max;
        *b = rgb_min;
        break;
    case 2:
        *r = rgb_min;
        *g = rgb_max;
        *b = rgb_min + rgb_adj;
        break;
    case 3:
        *r = rgb_min;
        *g = rgb_max - rgb_adj;
        *b = rgb_max;
        break;
    case 4:
        *r = rgb_min + rgb_adj;
        *g = rgb_min;
        *b = rgb_max;
        break;
    default:
        *r = rgb_max;
        *g = rgb_min;
        *b = rgb_max - rgb_adj;
        break;
    }
}

void bsp_rmt_task(void *arg)
{
    uint32_t red = 0;
    uint32_t green = 0;
    uint32_t blue = 0;
    uint16_t hue = 0;
    uint16_t start_rgb = 0;

    bsp_rmt_init();

    while (1)
    {
        // for (int i = 0; i < 3; i++) {
        //     for (int j = i; j < LED_NUMBERS; j += 3) {
        //         // Build RGB pixels
        //         hue = j * 360 / LED_NUMBERS + start_rgb;
        //         led_strip_hsv2rgb(hue, 100, 100, &red, &green, &blue);
        //         led_strip_pixels[j * 3 + 0] = green;
        //         led_strip_pixels[j * 3 + 1] = blue;
        //         led_strip_pixels[j * 3 + 2] = red;
        //     }
        //     // Flush RGB values to LEDs
        //     bsp_rmt_send_data(led_strip_pixels, sizeof(led_strip_pixels));
        //     ESP_ERROR_CHECK(rmt_tx_wait_all_done(tx_channel, portMAX_DELAY));
        //     vTaskDelay(pdMS_TO_TICKS(CHASE_SPEED_MS));

        //     // Clear RGB pixels
        //     memset(led_strip_pixels, 0, sizeof(led_strip_pixels));
        //     bsp_rmt_send_data(led_strip_pixels, sizeof(led_strip_pixels));
        //     ESP_ERROR_CHECK(rmt_tx_wait_all_done(tx_channel, portMAX_DELAY));
        //     vTaskDelay(pdMS_TO_TICKS(CHASE_SPEED_MS));
        // }
        // start_rgb += 60;

        // ESP_LOGI(TAG, "Start RGB: %d", start_rgb);

        // bsp_rmt_send_data(led_strip_pixels, sizeof(led_strip_pixels));
        // vTaskDelay(pdMS_TO_TICKS(1000));
        bsp_rmt_set_color(0);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
