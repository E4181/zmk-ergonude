#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#define P0_05_PORT DT_NODELABEL(gpio0)
#define P0_05_PIN  5

static const struct device *gpio0_dev;
static struct k_work_delayable matrix_monitor_work;
static int last_p0_05_state = -1;
static int problem_detected_count = 0;

// 强力重新配置P0.05，确保它是输入模式
static int force_p0_05_input_config(void)
{
    int ret;
    
    if (!device_is_ready(gpio0_dev)) {
        gpio0_dev = DEVICE_DT_GET(P0_05_PORT);
        if (!device_is_ready(gpio0_dev)) {
            LOG_ERR("GPIO0 device not ready");
            return -ENODEV;
        }
    }
    
    // 先尝试配置为输入下拉
    ret = gpio_pin_configure(gpio0_dev, P0_05_PIN, GPIO_INPUT | GPIO_PULL_DOWN);
    if (ret == 0) {
        LOG_INF("P0.05 configured as input with pull-down");
        return 0;
    }
    
    // 如果失败，尝试不带下拉
    LOG_WRN("P0.05 pull-down failed, trying without pull");
    ret = gpio_pin_configure(gpio0_dev, P0_05_PIN, GPIO_INPUT);
    if (ret == 0) {
        LOG_INF("P0.05 configured as input without pull");
        return 0;
    }
    
    LOG_ERR("Failed to configure P0.05 as input: %d", ret);
    return ret;
}

// 检查P0.05当前状态和配置
static void check_p0_05_status(void)
{
    if (!gpio0_dev || !device_is_ready(gpio0_dev)) {
        LOG_ERR("GPIO0 not ready for status check");
        return;
    }
    
    int current_state = gpio_pin_get(gpio0_dev, P0_05_PIN);
    
    // 检测状态变化
    if (last_p0_05_state != current_state) {
        LOG_INF("P0.05 state changed: %d -> %d", last_p0_05_state, current_state);
        last_p0_05_state = current_state;
    }
    
    // 如果状态异常（持续高电平），可能需要重新配置
    if (current_state == 1) {
        problem_detected_count++;
        if (problem_detected_count > 5) {
            LOG_WRN("P0.05 stuck high, forcing reconfiguration");
            force_p0_05_input_config();
            problem_detected_count = 0;
        }
    } else {
        problem_detected_count = 0;
    }
}

// 矩阵监控工作处理函数
static void matrix_monitor_handler(struct k_work *work)
{
    check_p0_05_status();
    
    // 持续监控，但频率逐渐降低
    static int monitor_count = 0;
    uint32_t delay_ms;
    
    if (monitor_count < 10) {
        delay_ms = 100;  // 前10次快速监控
    } else if (monitor_count < 30) {
        delay_ms = 500;  // 接下来20次中等频率
    } else {
        delay_ms = 2000; // 之后低频监控
    }
    
    monitor_count++;
    k_work_reschedule(&matrix_monitor_work, K_MSEC(delay_ms));
}

// 在矩阵扫描前后添加钩子函数
// 注意：这需要ZMK支持，如果不可用，我们会用监控方式
static void matrix_scan_pre_hook(void)
{
    // 在矩阵扫描前确保P0.05配置正确
    static int scan_count = 0;
    scan_count++;
    
    if (scan_count % 50 == 0) { // 每50次扫描检查一次
        check_p0_05_status();
    }
}

static int gpio_p0_05_fix_init(void)
{
    LOG_INF("Initializing enhanced P0.05 matrix fix");
    
    // 获取GPIO设备
    gpio0_dev = DEVICE_DT_GET(P0_05_PORT);
    if (!device_is_ready(gpio0_dev)) {
        LOG_ERR("GPIO0 device not ready at init");
        return -ENODEV;
    }
    
    // 立即配置P0.05
    int ret = force_p0_05_input_config();
    if (ret != 0) {
        LOG_ERR("Initial P0.05 configuration failed");
    }
    
    // 记录初始状态
    last_p0_05_state = gpio_pin_get(gpio0_dev, P0_05_PIN);
    LOG_INF("P0.05 initial state: %d", last_p0_05_state);
    
    // 启动矩阵监控
    k_work_init_delayable(&matrix_monitor_work, matrix_monitor_handler);
    k_work_reschedule(&matrix_monitor_work, K_MSEC(100));
    
    LOG_INF("P0.05 matrix fix initialized");
    return 0;
}

// 使用更高的优先级确保早期初始化
SYS_INIT(gpio_p0_05_fix_init, POST_KERNEL, 40);