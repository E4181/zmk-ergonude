#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#define P0_05_PORT DT_NODELABEL(gpio0)
#define P0_05_PIN  5

static const struct device *gpio0_dev;
static struct k_work_delayable last_resort_work;

// 最后手段：完全绕过ZMK矩阵扫描，直接实现P0.05行的扫描
static void last_resort_scan(void)
{
    static int scan_count = 0;
    
    if (!gpio0_dev || !device_is_ready(gpio0_dev)) {
        return;
    }
    
    // 强力确保P0.05是输入模式
    gpio_pin_configure(gpio0_dev, P0_05_PIN, GPIO_INPUT | GPIO_PULL_DOWN);
    
    // 读取当前状态
    int state = gpio_pin_get(gpio0_dev, P0_05_PIN);
    
    scan_count++;
    
    // 减少日志输出
    if (scan_count <= 10 || (scan_count % 100 == 0)) {
        LOG_INF("P0.05 last resort scan #%d: state=%d", scan_count, state);
    }
    
    // 如果检测到高电平（按键按下），可以在这里处理按键事件
    // 注意：这需要与ZMK的矩阵扫描集成，比较复杂
}

static void last_resort_handler(struct k_work *work)
{
    last_resort_scan();
    
    // 非常高的扫描频率
    k_work_reschedule(&last_resort_work, K_MSEC(20)); // 50Hz扫描
}

static int gpio_p0_05_last_resort_init(void)
{
    LOG_INF("=== STARTING P0.05 LAST RESORT ===");
    
    gpio0_dev = DEVICE_DT_GET(P0_05_PORT);
    
    // 立即开始扫描
    last_resort_scan();
    
    // 启动持续扫描
    k_work_init_delayable(&last_resort_work, last_resort_handler);
    k_work_reschedule(&last_resort_work, K_MSEC(10));
    
    return 0;
}

SYS_INIT(gpio_p0_05_last_resort_init, POST_KERNEL, 20);