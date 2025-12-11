#include "bsp_uart.h"
#include "esp_log.h"
#include "bsp_lcd.h"
#include "display/logo.h"

#define TAG "BSP_UART"

uint8_t rx_buf[RX_BUF_SIZE];

void bsp_uart_init(uart_port_t uart_num, uint32_t baudrate, uint8_t rx_pin, uint8_t tx_pin)
{
    uart_config_t uart_config = {
        .baud_rate = baudrate,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    };
    uart_param_config(uart_num, &uart_config);
    uart_set_pin(uart_num, tx_pin, rx_pin, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    uart_driver_install(uart_num, RX_BUF_SIZE, TX_BUF_SIZE, 0, NULL, 0);
}

void uart1_rx_task(void *pvParameters)
{
    bsp_uart_init(UART_NUM_1, 115200, 2, 1);

    uint8_t data[RX_BUF_SIZE] = {0};
    // uint8_t temp[55] = {0};
    char str[] = "esp32_restart";
    char red[] = "red";
    char green[] = "green";
    char blue[] = "blue";
    char logo[] = "logo";

    while (1)
    {
        int len = uart_read_bytes(UART_NUM_1, data, RX_BUF_SIZE, 10 / portTICK_PERIOD_MS);
        if (len > 0)
        {
            // Process received data
            data[len] = 0;

            ESP_LOGI(TAG, "Received %d bytes: %s", len, data);

            // sprintf((char *)temp, "Received %d bytes\r\n", len);
            // uart_write_bytes(UART_NUM_1, (const char *)temp, strlen((const char *)temp));
            // uart_write_bytes(UART_NUM_1, (const char *)"recieved data: ", strlen("received data: "));
            // uart_write_bytes(UART_NUM_1, (const char *)data, len);

            if (!strcmp((const char *)data, str))
            {
                ESP_LOGI(TAG, "Recvieved restart command, it will restart in 10 second...");
                for (int i = 10; i >= 0; i--)
                {
                    ESP_LOGI(TAG, "Restarting in %d seconds...", i);
                    vTaskDelay(1000 / portTICK_PERIOD_MS);
                }
                uart_flush(UART_NUM_1);
                esp_restart();
            }
            else if (!strcmp((const char *)data, red))
            {
                bsp_lcd_set_color(0xF800); // 红色
                uart_write_bytes(UART_NUM_1, (const char *)"set color red\r\n", strlen("set color red\r\n"));
            }
            else if (!strcmp((const char *)data, green))
            {
                bsp_lcd_set_color(0x07E0); // 绿色
                uart_write_bytes(UART_NUM_1, (const char *)"set color green\r\n", strlen("set color green\r\n"));
            }
            else if (!strcmp((const char *)data, blue))
            {
                bsp_lcd_set_color(0x001F); // 蓝色
                uart_write_bytes(UART_NUM_1, (const char *)"set color blue\r\n", strlen("set color blue\r\n"));
            }
            else if (!strcmp((const char *)data, logo))
            {
                bsp_lcd_draw_image(0, 0, 240, 240, (const uint16_t *)logo_en_240x240_lcd);
                uart_write_bytes(UART_NUM_1, (const char *)"show logo\r\n", strlen("show logo\r\n"));
            }


            uart_flush(UART_NUM_1);
        }
    }
}