#include "bsp_controls.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define TAG "BSP_CONTROLS"

static bool s_ac_on = false;
static bool s_light_on = false;
static bool s_curtain_open = false;

esp_err_t bsp_controls_init(void)
{
    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = (1ULL << AC_GPIO) |
                        (1ULL << LIGHT_GPIO) |
                        (1ULL << CURTAIN_OPEN_GPIO) |
                        (1ULL << CURTAIN_CLOSE_GPIO),
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en = GPIO_PULLUP_DISABLE,
    };
    esp_err_t ret = gpio_config(&io_conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "GPIO config failed: %d", ret);
        return ret;
    }

    gpio_set_level(AC_GPIO, 0);
    gpio_set_level(LIGHT_GPIO, 0);
    gpio_set_level(CURTAIN_OPEN_GPIO, 0);
    gpio_set_level(CURTAIN_CLOSE_GPIO, 0);

    ESP_LOGI(TAG, "Controls initialized: AC=GPIO%d, Light=GPIO%d, Curtain=GPIO%d/GPIO%d",
             AC_GPIO, LIGHT_GPIO, CURTAIN_OPEN_GPIO, CURTAIN_CLOSE_GPIO);
    return ESP_OK;
}

bool bsp_ac_get_state(void)
{
    return s_ac_on;
}

void bsp_ac_toggle(void)
{
    s_ac_on = !s_ac_on;
    gpio_set_level(AC_GPIO, s_ac_on ? 1 : 0);
    ESP_LOGI(TAG, "AC %s", s_ac_on ? "ON" : "OFF");
}

bool bsp_light_get_state(void)
{
    return s_light_on;
}

void bsp_light_toggle(void)
{
    s_light_on = !s_light_on;
    gpio_set_level(LIGHT_GPIO, s_light_on ? 1 : 0);
    ESP_LOGI(TAG, "Light %s", s_light_on ? "ON" : "OFF");
}

bool bsp_curtain_is_open(void)
{
    return s_curtain_open;
}

void bsp_curtain_open(void)
{
    if (s_curtain_open) {
        ESP_LOGI(TAG, "Curtain already open");
        return;
    }
    gpio_set_level(CURTAIN_OPEN_GPIO, 1);
    vTaskDelay(pdMS_TO_TICKS(500));
    gpio_set_level(CURTAIN_OPEN_GPIO, 0);
    s_curtain_open = true;
    ESP_LOGI(TAG, "Curtain opened");
}

void bsp_curtain_close(void)
{
    if (!s_curtain_open) {
        ESP_LOGI(TAG, "Curtain already closed");
        return;
    }
    gpio_set_level(CURTAIN_CLOSE_GPIO, 1);
    vTaskDelay(pdMS_TO_TICKS(500));
    gpio_set_level(CURTAIN_CLOSE_GPIO, 0);
    s_curtain_open = false;
    ESP_LOGI(TAG, "Curtain closed");
}
