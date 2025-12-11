#pragma once

#include "driver/uart.h"
#include "driver/gpio.h"
#include <string.h>

#define RX_BUF_SIZE 256
#define TX_BUF_SIZE 256

#define ESP32S3_RESTART "esp32s3_restart"

extern uint8_t rx_buf[RX_BUF_SIZE];

void bsp_uart_init(uart_port_t uart_num, uint32_t baudrate, uint8_t rx_pin, uint8_t tx_pin);
void uart1_rx_task(void *pvParameters);
