#include "bsp_dht11.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "driver/rmt_rx.h"
#include "freertos/semphr.h"
#include "rom/ets_sys.h"
#include "esp_rom_gpio.h"

#define TAG "BSP_DHT11"

#define DHT11_RMT_RESOLUTION_HZ 1000000   /* 1MHz = 1us/tick */
#define DHT11_RMT_SYMBOLS       64

static rmt_channel_handle_t dht11_rx_channel = NULL;
static SemaphoreHandle_t dht11_rx_sem = NULL;
static rmt_symbol_word_t dht11_rx_symbols[DHT11_RMT_SYMBOLS];
static size_t dht11_rx_num_symbols = 0;

/* RMT RX done callback (ISR context) */
static bool IRAM_ATTR dht11_rmt_rx_done_cb(rmt_channel_handle_t channel,
    const rmt_rx_done_event_data_t *edata, void *user_data)
{
    *(size_t *)user_data = edata->num_symbols;
    BaseType_t higher_priority_woken = pdFALSE;
    xSemaphoreGiveFromISR(dht11_rx_sem, &higher_priority_woken);
    return higher_priority_woken == pdTRUE;
}

static esp_err_t bsp_dht11_rmt_init(void)
{
    rmt_rx_channel_config_t rx_config = {
        .gpio_num = DHT11_GPIO,
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = DHT11_RMT_RESOLUTION_HZ,
        .mem_block_symbols = DHT11_RMT_SYMBOLS,
    };

    esp_err_t ret = rmt_new_rx_channel(&rx_config, &dht11_rx_channel);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "create RMT RX failed: %d", ret);
        return ret;
    }

    dht11_rx_sem = xSemaphoreCreateBinary();
    if (!dht11_rx_sem) return ESP_ERR_NO_MEM;

    rmt_rx_event_callbacks_t cbs = {
        .on_recv_done = dht11_rmt_rx_done_cb,
    };
    ret = rmt_rx_register_event_callbacks(dht11_rx_channel, &cbs, &dht11_rx_num_symbols);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "register RX callback failed: %d", ret);
        return ret;
    }

    ret = rmt_enable(dht11_rx_channel);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "enable RMT RX failed: %d", ret);
        return ret;
    }
    return ESP_OK;
}

/* Decode RMT captured symbols into humidity/temperature */
static esp_err_t dht11_decode_rmt(const rmt_symbol_word_t *symbols, size_t num,
                                  int *humidity, float *temperature)
{
    /* 40 data bits starting from symbols[0].
     * RMT does not capture the DHT11 response pulse (80us LOW+HIGH),
     * so sym[0] is already the first data bit (humidity MSB).
     * sym[40] is a stop marker (duration1 = 0). */
    if (num < 40) {
        ESP_LOGW(TAG, "too few symbols: %d", num);
        return ESP_FAIL;
    }

    uint8_t data[5] = {0};
    for (int i = 0; i < 40; i++) {
        data[i / 8] <<= 1;
        /* bit 1 = ~72us high, bit 0 = ~25us high; threshold = 50us */
        if (symbols[i].duration1 > 50)
            data[i / 8] |= 1;
    }

    uint8_t sum = data[0] + data[1] + data[2] + data[3];
    if (data[4] != sum) {
        ESP_LOGW(TAG, "checksum failed");
        return ESP_FAIL;
    }

    *humidity    = data[0];
    *temperature = (float)data[2] + (float)data[3] * 0.1f;
    return ESP_OK;
}

esp_err_t bsp_dht11_init(void)
{
    esp_err_t ret = bsp_dht11_rmt_init();
    if (ret != ESP_OK) return ret;

    esp_rom_gpio_pad_select_gpio(DHT11_GPIO);
    gpio_set_direction(DHT11_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_level(DHT11_GPIO, 1);

    ESP_LOGI(TAG, "DHT11 initialized on GPIO%d with RMT", DHT11_GPIO);
    return ESP_OK;
}

esp_err_t bsp_dht11_read(int *humidity, float *temperature)
{
    if (humidity == NULL || temperature == NULL)
        return ESP_ERR_INVALID_ARG;

    /* 1. Start signal: pull LOW > 18ms */
    gpio_set_direction(DHT11_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_level(DHT11_GPIO, 0);
    vTaskDelay(pdMS_TO_TICKS(19));

    /* 2. Release bus, switch to input with pull-up */
    gpio_set_level(DHT11_GPIO, 1);
    gpio_set_direction(DHT11_GPIO, GPIO_MODE_INPUT);
    gpio_set_pull_mode(DHT11_GPIO, GPIO_PULLUP_ONLY);
    ets_delay_us(5);

    /* 3. Start RMT reception */
    dht11_rx_num_symbols = 0;
    rmt_receive_config_t rx_config = {
        .signal_range_min_ns = 2000,      /* ignore glitches < 2us (max allowed is 3187ns at 1MHz) */
        .signal_range_max_ns = 2500000,   /* 2.5ms idle timeout */
    };

    xSemaphoreTake(dht11_rx_sem, 0);  /* flush stale semaphore */
    if (rmt_receive(dht11_rx_channel, dht11_rx_symbols,
                    sizeof(dht11_rx_symbols), &rx_config) != ESP_OK) {
        ESP_LOGW(TAG, "rmt_receive failed");
        goto fail;
    }

    /* 4. Wait for RMT to finish capturing */
    if (xSemaphoreTake(dht11_rx_sem, pdMS_TO_TICKS(100)) != pdTRUE) {
        ESP_LOGW(TAG, "RMT receive timeout (no response from DHT11)");
        goto fail;
    }

    /* 5. Decode captured symbols */
    if (dht11_decode_rmt(dht11_rx_symbols, dht11_rx_num_symbols,
                         humidity, temperature) != ESP_OK) {
        goto fail;
    }

    gpio_set_direction(DHT11_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_level(DHT11_GPIO, 1);
    return ESP_OK;

fail:
    gpio_set_direction(DHT11_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_level(DHT11_GPIO, 1);
    *humidity    = 0;
    *temperature = 0;
    return ESP_FAIL;
}

void bsp_dht11_task(void *arg)
{
    int humidity = 0;
    float temperature = 0;
    int fail_count = 0;

    vTaskDelay(pdMS_TO_TICKS(1500));

    while (1) {
        esp_err_t ret = ESP_FAIL;
        for (int i = 0; i < 3; i++) {
            ret = bsp_dht11_read(&humidity, &temperature);
            if (ret == ESP_OK) break;
            vTaskDelay(pdMS_TO_TICKS(200));
        }

        if (ret == ESP_OK) {
            ESP_LOGI(TAG, "Temp: %.1fC, Humidity: %d%%",
                     temperature, humidity);
            fail_count = 0;
        } else {
            fail_count++;
            ESP_LOGE(TAG, "read failed (%d)", fail_count);
        }
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}
