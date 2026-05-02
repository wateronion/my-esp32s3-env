#include "bsp_dht11.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "rom/ets_sys.h"
#include "esp_rom_gpio.h"

#define TAG "BSP_DHT11"

#define DHT11_TIMEOUT  200
#define DHT11_DELAY_US 10

static void dht11_output(void)
{
    esp_rom_gpio_pad_select_gpio(DHT11_GPIO);
    gpio_set_direction(DHT11_GPIO, GPIO_MODE_OUTPUT);
}

static void dht11_input(void)
{
    esp_rom_gpio_pad_select_gpio(DHT11_GPIO);
    gpio_set_direction(DHT11_GPIO, GPIO_MODE_INPUT);
    gpio_set_pull_mode(DHT11_GPIO, GPIO_PULLUP_ONLY);
}

/* Wait for GPIO to reach expected level, returns false on timeout */
static bool dht11_wait_level(int level, uint32_t timeout_us)
{
    uint32_t t = 0;
    while (gpio_get_level(DHT11_GPIO) == level && t < timeout_us) {
        ets_delay_us(1);
        t++;
    }
    return (t < timeout_us);
}

/* Read one byte from DHT11 */
static uint8_t dht11_read_byte(void)
{
    uint8_t data = 0;
    for (int i = 0; i < 8; i++) {
        /* Wait for LOW (start of bit) */
        if (!dht11_wait_level(0, DHT11_TIMEOUT))
            return 0;

        ets_delay_us(35); /* sample point: 35us into HIGH */

        /* If still HIGH at 35us → bit=1, else bit=0 */
        data <<= 1;
        if (gpio_get_level(DHT11_GPIO) == 1)
            data |= 1;

        /* Wait for HIGH to end before next bit */
        if (!dht11_wait_level(1, DHT11_TIMEOUT))
            return 0;
    }
    return data;
}

esp_err_t bsp_dht11_init(void)
{
    dht11_output();
    gpio_set_level(DHT11_GPIO, 1);
    ESP_LOGI(TAG, "DHT11 initialized on GPIO%d", DHT11_GPIO);
    return ESP_OK;
}

esp_err_t bsp_dht11_read(float *humidity, float *temperature)
{
    if (humidity == NULL || temperature == NULL)
        return ESP_ERR_INVALID_ARG;

    /* 1. Start signal: pull LOW > 18ms */
    dht11_output();
    gpio_set_level(DHT11_GPIO, 0);
    vTaskDelay(pdMS_TO_TICKS(19));

    /* 2. Release bus, pull HIGH, switch to input */
    gpio_set_level(DHT11_GPIO, 1);
    dht11_input();
    ets_delay_us(30);

    /* 3. Check sensor response (LOW ~80us → HIGH ~80us) */
    if (gpio_get_level(DHT11_GPIO) != 0) {
        ESP_LOGW(TAG, "no response from DHT11");
        goto fail;
    }
    if (!dht11_wait_level(0, DHT11_TIMEOUT) ||
        !dht11_wait_level(1, DHT11_TIMEOUT)) {
        ESP_LOGW(TAG, "response timeout");
        goto fail;
    }

    /* 4. Read 40 bits: 2 bytes humidity + 2 bytes temperature + 1 byte checksum */
    uint8_t data[5];
    for (int i = 0; i < 5; i++) {
        data[i] = dht11_read_byte();
    }

    /* 5. Verify checksum */
    uint8_t sum = data[0] + data[1] + data[2] + data[3];
    if (data[4] != sum) {
        ESP_LOGW(TAG, "checksum failed: %02x != %02x", data[4], sum);
        goto fail;
    }

    /* 6. Extract values including decimal parts */
    /* data[0]=humidity int, data[1]=humidity decimal, data[2]=temp int, data[3]=temp decimal */
    *humidity    = (float)data[0] + (float)data[1] * 0.1f;
    *temperature = (float)data[2] + (float)data[3] * 0.1f;

    dht11_output();
    gpio_set_level(DHT11_GPIO, 1);
    return ESP_OK;

fail:
    dht11_output();
    gpio_set_level(DHT11_GPIO, 1);
    *humidity    = 0;
    *temperature = 0;
    return ESP_FAIL;
}

void bsp_dht11_task(void *arg)
{
    float humidity = 0, temperature = 0;
    int fail_count = 0;

    vTaskDelay(pdMS_TO_TICKS(1500));

    while (1) {
        esp_err_t ret = bsp_dht11_read(&humidity, &temperature);
        if (ret == ESP_OK) {
            ESP_LOGI(TAG, "Temp: %.1fC, Humidity: %.1f%%",
                     temperature, humidity);
            fail_count = 0;
        } else {
            // fail_count++;
            // ESP_LOGE(TAG, "read failed (%d)", fail_count);
        }
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}
