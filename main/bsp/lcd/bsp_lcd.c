#include "bsp_lcd.h"
#include "hal/i2c_types.h"
#include "lvgl.h"
#include "driver/spi_master.h"
#include "driver/i2c_master.h"
#include "esp_check.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_touch_ft5x06.h"
#include <stdint.h>
#include "esp_lv_adapter.h"


#define TAG "BSP_LCD"
#define TOUCH_MAX_POINTS  1

static esp_lcd_panel_handle_t panel_handle = NULL; // LCD 面板句柄
static i2c_master_bus_handle_t bus_handle = NULL; // I2C 总线句柄

void bsp_lcd_display_init(void)
{
    ESP_LOGI(TAG, "Initialize SPI bus");
    spi_bus_config_t buscfg = {
        .sclk_io_num = LCD_PIN_NUM_SCLK,                              // 连接 LCD SCK（SCL） 信号的 IO 编号
        .mosi_io_num = LCD_PIN_NUM_MOSI,                              // 连接 LCD MOSI（SDO、SDA） 信号的 IO 编号
        .miso_io_num = -1,                                            // 连接 LCD MISO（SDI） 信号的 IO 编号，如果不需要从 LCD 读取数据，可以设为 `-1`
        .quadwp_io_num = -1,                                          // 必须设置且为 `-1`
        .quadhd_io_num = -1,                                          // 必须设置且为 `-1`
        // .max_transfer_sz = LCD_WIDTH * LCD_HEIGHT * sizeof(uint16_t), // 表示 SPI 单次传输允许的最大字节数上限，通常设为全屏大小即可
        .max_transfer_sz = 4096, // 表示 SPI 单次传输允许的最大字节数上限，通常设为全屏大小即可
    };

    ESP_ERROR_CHECK(spi_bus_initialize(LCD_HOST, &buscfg, SPI_DMA_CH_AUTO));
    // 第 1 个参数表示使用的 SPI 主机 ID，和后续创建接口设备时保持一致
    // 第 3 个参数表示使用的 DMA 通道号，默认设置为 `SPI_DMA_CH_AUTO` 即可

    ESP_LOGI(TAG, "Install panel IO");
    esp_lcd_panel_io_handle_t io_handle = NULL; // LCD 面板 IO 句柄
    esp_lcd_panel_io_spi_config_t io_config = {
        .dc_gpio_num = LCD_PIN_NUM_DC, // 连接 LCD DC（RS） 信号的 IO 编号，可以设为 `-1` 表示不使用
        .cs_gpio_num = LCD_PIN_NUM_CS, // 连接 LCD CS 信号的 IO 编号，可以设为 `-1` 表示不使用
        .pclk_hz = 40 * 1000 * 1000,   // SPI 的时钟频率（Hz），ESP 最高支持 80M（SPI_MASTER_FREQ_80M）
                                       // 需根据 LCD 驱动 IC 的数据手册确定其最大值
        .lcd_cmd_bits = 8,             // 单位 LCD 命令的比特数，应为 8 的整数倍
        .lcd_param_bits = 8,           // 单位 LCD 参数的比特数，应为 8 的整数倍
        .spi_mode = 2,                 // SPI 模式（0-3），需根据 LCD 驱动 IC 的数据手册以及硬件的配置确定（如 IM[3:0]）
        .trans_queue_depth = 10,       // SPI 设备传输数据的队列深度，一般设为 10 即可
        // .on_color_trans_done = example_on_color_trans_dome, // 单次调用 `esp_lcd_panel_draw_bitmap()` 传输完成后的回调函数
        // .user_ctx = &example_user_ctx,                      // 传给回调函数的用户参数
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)LCD_HOST, &io_config, &io_handle));

    ESP_LOGI(TAG, "Initialize LCD panel");

    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = LCD_PIN_NUM_RST,      // 连接 LCD RST 信号的 IO 编号，可以设为 `-1` 表示不使用
        // .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_BGR,
        // .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,
        .data_endian = LCD_RGB_ENDIAN_RGB,
        // .data_endian = LCD_RGB_ENDIAN_BGR,
        .bits_per_pixel = 16, // LCD 每个像素的数据位宽
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_st7789(io_handle, &panel_config, &panel_handle));

    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle)); // 复位 LCD 面板
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));  // 初始化 LCD 面板

    // ST7789 240×320 初始化指令（解决分辨率错位）
    // const uint8_t st7789_init_cmds[] = {
    //     0x36, 1, 0x00,  // MADCTL：MY=0, MX=0, MV=0 → 基础竖屏（先恢复原始方向）
    //     0x3A, 1, 0x05,  // COLMOD：16位色
    //     0x2A, 4, 0x00, 0x40, 0x00, 0xEF,  // CASET：列地址范围（40-279）→ 240列（279-40+1=240）
    //     0x2B, 4, 0x00, 0x00, 0x01, 0x3F,  // RASET：行地址范围（0-319）→ 320行（319-0+1=320）
    //     // 0x21, 0,        // INVON：开启颜色反转（根据面板调整，若颜色反了就注释）
    //     0x11, 0,        // SLPOUT：退出睡眠
    //     0x29, 0,        // DISPON：开启显示
    // };
    // for (int i = 0; i < sizeof(st7789_init_cmds);) {
    //     uint8_t cmd = st7789_init_cmds[i++];
    //     uint8_t param_len = st7789_init_cmds[i++];
    //     esp_lcd_panel_io_tx_param(io_handle, cmd, &st7789_init_cmds[i], param_len);
    //     i += param_len;
    // }

    // ESP_ERROR_CHECK(esp_lcd_panel_invert_color(panel_handle, true)); // 反转 LCD 面板的颜色
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true)); // 打开 LCD 显示
    // ESP_ERROR_CHECK(esp_lcd_panel_swap_xy(panel_handle, true));    // 交换 LCD 的 X 和 Y 轴
    ESP_ERROR_CHECK(esp_lcd_panel_mirror(panel_handle, true, true)); // 设置 LCD 的镜像

    bsp_lcd_set_color(0x0000); // 清屏

    esp_lv_adapter_config_t cfg = ESP_LV_ADAPTER_DEFAULT_CONFIG();
    ESP_ERROR_CHECK(esp_lv_adapter_init(&cfg));

    esp_lv_adapter_display_config_t disp_cfg = ESP_LV_ADAPTER_DISPLAY_SPI_WITH_PSRAM_DEFAULT_CONFIG(
        panel_handle,           // LCD 面板句柄
        io_handle,        // LCD 面板 IO 句柄（某些接口可为 NULL）
        240,             // 水平分辨率
        320,             // 垂直分辨率
        ESP_LV_ADAPTER_ROTATE_0 // 旋转角度
    );
    lv_display_t *disp = esp_lv_adapter_register_display(&disp_cfg);
    assert(disp != NULL);

    // 初始化触摸屏（FT5x06）相关的 I2C 总线和设备
    if (bus_handle != NULL) {
        return;
    }

    ESP_LOGI(TAG, "Initialize I2C bus (new driver)");
    i2c_master_bus_config_t bus_cfg = {
        .i2c_port = I2C_NUM_0,
        .sda_io_num = LCD_PIN_NUM_TOUCH_SDA,
        .scl_io_num = LCD_PIN_NUM_TOUCH_SCL,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_cfg, &bus_handle));

    esp_lcd_touch_handle_t touch_handle = NULL;
    esp_lcd_panel_io_handle_t tp_io_handle = NULL;
    esp_lcd_panel_io_i2c_config_t tp_io_config = ESP_LCD_TOUCH_IO_I2C_FT5x06_CONFIG();
    tp_io_config.scl_speed_hz = 100000;

    /* 必须传新驱动的 bus_handle，不能传 I2C_NUM_0，否则会走旧 I2C 驱动并触发 CONFLICT 崩溃 */
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_i2c(bus_handle, &tp_io_config, &tp_io_handle));

    esp_lcd_touch_config_t touch_cfg = {
        .x_max = LCD_WIDTH,
        .y_max = LCD_HEIGHT,
        .rst_gpio_num = LCD_PIN_NUM_TOUCH_RST,
        .int_gpio_num = LCD_PIN_NUM_TOUCH_INT,
        .levels = { 
            .reset = 0, 
            .interrupt = 0 
        },
        .flags = { 
            .swap_xy = 0, 
            .mirror_x = 1, 
            .mirror_y = 1
        },
    };
    ESP_ERROR_CHECK(esp_lcd_touch_new_i2c_ft5x06(tp_io_handle, &touch_cfg, &touch_handle));
    esp_lv_adapter_touch_config_t touch_adapter_cfg = ESP_LV_ADAPTER_TOUCH_DEFAULT_CONFIG(disp, touch_handle);
    lv_indev_t *touch = esp_lv_adapter_register_touch(&touch_adapter_cfg);
    assert(touch != NULL);

    ESP_ERROR_CHECK(esp_lv_adapter_start());

    // 步骤 5: 使用 LVGL API 绘制界面（需要先加锁）
    
}

void bsp_lcd_set_color(uint16_t color)
{
    // 预生成全屏颜色数据
    static uint16_t *full_screen_buf = NULL;
    if (full_screen_buf == NULL) {
        full_screen_buf = heap_caps_malloc(LCD_WIDTH * LCD_HEIGHT * sizeof(uint16_t), MALLOC_CAP_DMA);
        ESP_ERROR_CHECK(full_screen_buf != NULL ? ESP_OK : ESP_ERR_NO_MEM);
    }
    for (int i = 0; i < LCD_WIDTH * LCD_HEIGHT; i++) {
        full_screen_buf[i] = color;
    }

    ESP_ERROR_CHECK(esp_lcd_panel_draw_bitmap(panel_handle, 0, 0, LCD_WIDTH, LCD_HEIGHT, full_screen_buf));
}

void bsp_lcd_draw_image(int x, int y, int width, int height, const uint16_t *image_data)
{
    ESP_ERROR_CHECK(esp_lcd_panel_draw_bitmap(panel_handle, x, y, x + width, y + height, (uint16_t *)image_data));
}

void bsp_i2c_init(void)
{
    if (bus_handle != NULL) {
        return;
    }

    ESP_LOGI(TAG, "Initialize I2C bus (new driver)");
    i2c_master_bus_config_t bus_cfg = {
        .i2c_port = I2C_NUM_0,
        .sda_io_num = LCD_PIN_NUM_TOUCH_SDA,
        .scl_io_num = LCD_PIN_NUM_TOUCH_SCL,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_cfg, &bus_handle));
}

void bsp_lcd_touch_init(esp_lcd_touch_handle_t *ret_touch)
{
    bsp_i2c_init();
    
    esp_lcd_panel_io_handle_t tp_io_handle = NULL;
    esp_lcd_panel_io_i2c_config_t tp_io_config = ESP_LCD_TOUCH_IO_I2C_FT5x06_CONFIG();
    tp_io_config.scl_speed_hz = 100000;

    /* 必须传新驱动的 bus_handle，不能传 I2C_NUM_0，否则会走旧 I2C 驱动并触发 CONFLICT 崩溃 */
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_i2c(bus_handle, &tp_io_config, &tp_io_handle));

    esp_lcd_touch_config_t touch_cfg = {
        .x_max = LCD_WIDTH,
        .y_max = LCD_HEIGHT,
        .rst_gpio_num = LCD_PIN_NUM_TOUCH_RST,
        .int_gpio_num = LCD_PIN_NUM_TOUCH_INT,
        .levels = { 
            .reset = 0, 
            .interrupt = 0 
        },
        .flags = { 
            .swap_xy = 0, 
            .mirror_x = 0, 
            .mirror_y = 0
        },
    };
    ESP_ERROR_CHECK(esp_lcd_touch_new_i2c_ft5x06(tp_io_handle, &touch_cfg, ret_touch));
}

void bsp_lcd_touch_test(void)
{
    esp_lcd_touch_handle_t touch_handle = NULL;
    bsp_lcd_touch_init(&touch_handle);
    
    // 检查设备ID
    uint8_t data[4] = {0};
    esp_lcd_panel_io_handle_t io_handle;
    // 这里需要获取触摸的io_handle来直接读取寄存器
    
    ESP_LOGI(TAG, "Checking touch device ID...");
    
    // 等待触摸芯片稳定
    vTaskDelay(pdMS_TO_TICKS(500));
    
    while(1) {
        uint8_t touch_cnt = 0;
        uint16_t x = 0, y = 0;
        
        // 必须先用read_data更新缓冲区
        esp_lcd_touch_read_data(touch_handle);
        
        // 然后获取坐标
        if(esp_lcd_touch_get_coordinates(touch_handle, &x, &y, NULL, &touch_cnt, 1)) {
            if(touch_cnt > 0) {
                ESP_LOGI(TAG, "Touch! x=%d, y=%d", x, y);
            }
        }
        
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void bsp_lcd_set_rotation(lv_display_rotation_t rotation)
{
    switch (rotation)
    {
    case LV_DISPLAY_ROTATION_0:
        ESP_ERROR_CHECK(esp_lcd_panel_swap_xy(panel_handle, false));
        ESP_ERROR_CHECK(esp_lcd_panel_mirror(panel_handle, true, true));
        break;
    case LV_DISPLAY_ROTATION_90:
        ESP_ERROR_CHECK(esp_lcd_panel_swap_xy(panel_handle, true));
        ESP_ERROR_CHECK(esp_lcd_panel_mirror(panel_handle, true, false));
        break;
    case LV_DISPLAY_ROTATION_180:
        ESP_ERROR_CHECK(esp_lcd_panel_swap_xy(panel_handle, false));
        ESP_ERROR_CHECK(esp_lcd_panel_mirror(panel_handle, false, false));
        break;
    case LV_DISPLAY_ROTATION_270:
        ESP_ERROR_CHECK(esp_lcd_panel_swap_xy(panel_handle, true));
        ESP_ERROR_CHECK(esp_lcd_panel_mirror(panel_handle, false, true));
        break;
    default:
        break;
    }
}

// void bsp_display_task(void *arg)
// {
//     bsp_lcd_init();
// }


