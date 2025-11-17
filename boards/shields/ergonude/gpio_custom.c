#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

// P0.05 引脚定义
#define P0_05_PORT DT_NODELABEL(gpio0)
#define P0_05_PIN  5

static const struct device *gpio0_dev;
static struct k_work_delayable reconfig_work;

// 强制配置P0.05为GPIO输入（带下拉）
static int force_p0_05_gpio_config(void)
{
    int ret;
    
    // 获取GPIO0设备
    gpio0_dev = DEVICE_DT_GET(P0_05_PORT);
    if (!device_is_ready(gpio0_dev)) {
        LOG_ERR("GPIO0 device not ready");
        return -ENODEV;
    }
    
    // 使用底层GPIO配置，配置为输入带下拉
    ret = gpio_pin_configure(gpio0_dev, P0_05_PIN, 
                           GPIO_INPUT | GPIO_PULL_DOWN | GPIO_ACTIVE_HIGH);
    if (ret < 0) {
        LOG_ERR("Failed to configure P0.05 as GPIO input with pull-down: %d", ret);
        return ret;
    }
    
    LOG_INF("P0.05 successfully forced to GPIO input with pull-down");
    
    // 验证配置和初始状态
    int pin_state = gpio_pin_get(gpio0_dev, P0_05_PIN);
    LOG_INF("P0.05 initial state: %d (should be 0 with pull-down)", pin_state);
    
    return 0;
}

// 检查P0.05当前配置状态（调试用）
static void check_p0_05_status(void)
{
    if (!gpio0_dev || !device_is_ready(gpio0_dev)) {
        LOG_ERR("GPIO0 not ready for status check");
        return;
    }
    
    // 读取引脚状态
    int state = gpio_pin_get(gpio0_dev, P0_05_PIN);
    
    LOG_INF("P0.05 current state: %d", state);
}

// 定期重新配置P0.05（防止被其他驱动修改）
static void periodic_reconfig_handler(struct k_work *work)
{
    static int reconfig_count = 0;
    int ret;
    
    // 强制重新配置
    ret = force_p0_05_gpio_config();
    
    if (ret == 0) {
        reconfig_count++;
        
        // 减少日志输出频率，避免干扰
        if (reconfig_count <= 3 || reconfig_count % 20 == 0) {
            LOG_DBG("P0.05 reconfiguration successful, count: %d", reconfig_count);
        }
    } else {
        LOG_ERR("P0.05 reconfiguration failed: %d", ret);
    }
    
    // 动态调整重配频率
    uint32_t delay_ms;
    if (reconfig_count < 5) {
        delay_ms = 100;      // 前期快速重配
    } else if (reconfig_count < 15) {
        delay_ms = 500;      // 中期中等频率
    } else {
        delay_ms = 2000;     // 后期较低频率
    }
    
    k_work_reschedule(&reconfig_work, K_MSEC(delay_ms));
}

// 初始化函数
static int gpio_custom_init(void)
{
    LOG_INF("Initializing GPIO custom fix for P0.05 (pull-down)");
    
    // 等待系统基本初始化完成
    k_msleep(50);
    
    // 立即应用P0.05修复
    int ret = force_p0_05_gpio_config();
    if (ret < 0) {
        LOG_ERR("Initial P0.05 configuration failed, will retry periodically");
    }
    
    // 启动定期重配工作
    k_work_init_delayable(&reconfig_work, periodic_reconfig_handler);
    
    // 首次延迟后开始定期重配
    k_work_reschedule(&reconfig_work, K_MSEC(100));
    
    LOG_INF("GPIO custom fix initialized successfully");
    
    return 0;
}

// 导出引脚状态检查函数（可用于调试shell命令）
void gpio_custom_check_p0_05(void)
{
    check_p0_05_status();
}

// 手动重新配置函数（可用于调试shell命令）
int gpio_custom_reconfig_p0_05(void)
{
    LOG_INF("Manual P0.05 reconfiguration requested");
    return force_p0_05_gpio_config();
}

// 使用POST_KERNEL优先级，确保在大多数驱动之后初始化
SYS_INIT(gpio_custom_init, POST_KERNEL, 90);