#include "bsp_led.h"

void led_init(void)
{
    gpio_config_t io_conf = {0};

    // 配置IO为通用IO
    esp_rom_gpio_pad_select_gpio(LED_PIN);

    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = (1ULL << LED_PIN);
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;

    gpio_config(&io_conf);

    gpio_set_level(LED_PIN, 1);
}

void led_on(void)
{
    gpio_set_level(LED_PIN, 0);
}

void led_off(void)
{
    gpio_set_level(LED_PIN, 1);
}