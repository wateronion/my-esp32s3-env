#include "bsp_lcd.h"
#include "lvgl.h"
#include "driver/spi_master.h"
#include "driver/i2c_master.h"
#include "esp_check.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_touch.h"
#include "esp_lcd_touch_ft5x06.h"


#define TAG "BSP_LCD"
#define TOUCH_MAX_POINTS  1

static esp_lcd_panel_handle_t panel_handle = NULL;
static esp_lcd_touch_handle_t touch_handle = NULL;

void bsp_lcd_display_init(void)
{
    ESP_LOGI(TAG, "Initialize SPI bus");
    spi_bus_config_t buscfg = {
        .sclk_io_num = LCD_PIN_NUM_SCLK,                              // 连接 LCD SCK（SCL） 信号的 IO 编号
        .mosi_io_num = LCD_PIN_NUM_MOSI,                              // 连接 LCD MOSI（SDO、SDA） 信号的 IO 编号
        .miso_io_num = -1,                                            // 连接 LCD MISO（SDI） 信号的 IO 编号，如果不需要从 LCD 读取数据，可以设为 `-1`
        .quadwp_io_num = -1,                                          // 必须设置且为 `-1`
        .quadhd_io_num = -1,                                          // 必须设置且为 `-1`
        .max_transfer_sz = LCD_WIDTH * LCD_HEIGHT * sizeof(uint16_t), // 表示 SPI 单次传输允许的最大字节数上限，通常设为全屏大小即可
    };

    ESP_ERROR_CHECK(spi_bus_initialize(LCD_HOST, &buscfg, SPI_DMA_CH_AUTO));
    // 第 1 个参数表示使用的 SPI 主机 ID，和后续创建接口设备时保持一致
    // 第 3 个参数表示使用的 DMA 通道号，默认设置为 `SPI_DMA_CH_AUTO` 即可

    ESP_LOGI(TAG, "Install panel IO");
    esp_lcd_panel_io_handle_t io_handle = NULL;
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
        // .data_endian = LCD_RGB_ENDIAN_RGB,
        .data_endian = LCD_RGB_ENDIAN_BGR,
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

void bsp_lcd_touch_init(void)
{
    
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


