#include "bsp_timer.h"

#define TAG "BSP_TIMER"

// 定时器回调函数
static bool IRAM_ATTR TimerCallback(gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void *user_data)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    QueueHandle_t xQueue = (QueueHandle_t)user_data;

    static int time_count = 0;

    time_count++;

    xQueueSendFromISR(xQueue, &time_count, &xHigherPriorityTaskWoken);

    return (xHigherPriorityTaskWoken == pdTRUE);
}
/**
 * @brief 定时器初始化
 * 
 * @param resolution_hz  定时器分辨率
 * @param alarm_count     报警次数
 * @return QueueHandle_t 
 */
QueueHandle_t TimerInitConfig(uint32_t resolution_hz, uint64_t alarm_count)
{
    // 定义一个通用定时器
    gptimer_handle_t gptimer = NULL;

    QueueHandle_t xQueue = xQueueCreate(10, sizeof(int));
    if (!xQueue)
    {
        ESP_LOGE(TAG, "Queue create failed");
        return NULL;
    }

    // 初始化定时器
    gptimer_config_t gptimer_config = {
        .clk_src = GPTIMER_CLK_SRC_DEFAULT,
        .direction = GPTIMER_COUNT_UP,
        .resolution_hz = resolution_hz,
    };
    // 创建定时器实例
    ESP_ERROR_CHECK(gptimer_new_timer(&gptimer_config, &gptimer));

    // 绑定回调函数
    gptimer_event_callbacks_t callbacks = {
        .on_alarm = TimerCallback,
    };
    // 定时器的回调函数绑定到队列
    gptimer_register_event_callbacks(gptimer, &callbacks, xQueue);

    // 在对定时器进行控制之前，需要先使能定时器
    ESP_ERROR_CHECK(gptimer_enable(gptimer));

    // 设置定时器报警
    gptimer_alarm_config_t alarm_config = {
        .alarm_count = alarm_count,
        .flags.auto_reload_on_alarm = true,
        .reload_count = 0,
    };
    ESP_ERROR_CHECK(gptimer_set_alarm_action(gptimer, &alarm_config));
    ESP_ERROR_CHECK(gptimer_start(gptimer));

    return xQueue;
}

void timer_task(void *pvParameters)
{
    int number = 0;
    QueueHandle_t my_queue = 0;
    my_queue = TimerInitConfig(1000000,1000000);

    while (1)
    {
        if (xQueueReceive(my_queue, &number, portMAX_DELAY))
        {
            ESP_LOGI(TAG, "number = %d", number);
        }
        else
        {
            ESP_LOGW(TAG, "queue receive failed");
        }
    }
}