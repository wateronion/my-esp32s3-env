idf_component_register(
SRCS            "main.c"
                "bsp/led/bsp_led.c"
                "bsp/uart/bsp_uart.c"
                "bsp/timer/bsp_timer.c"
                "bsp/pwm/bsp_pwm.c"
                "bsp/rmt/bsp_rmt.c"
                "bsp/lcd/bsp_lcd.c"
                "bsp/user_lvgl/ulvgl.c"
PRIV_REQUIRES   spi_flash
                driver
                esp_driver_rmt
                esp_lcd
INCLUDE_DIRS    "."
                "bsp/led"
                "bsp/uart"
                "bsp/timer"
                "bsp/pwm"
                "bsp/rmt"
                "bsp/lcd"
                "display"
                "bsp/user_lvgl"
)
