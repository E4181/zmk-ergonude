#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/printk.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

// P0.05 引脚定义
#define P0_05_PORT DT_NODELABEL(gpio0)
#define P0_05_PIN  5

static const struct device *gpio0_dev;
static struct k_work_delayable reconfig_work;

// 强制配置P0.05为GPIO输入（带上拉）
static int force_p0_05_gpio_config(void)
{
    int ret;
    
    // 获取GPIO0设备
    gpio0_dev = DEVICE_DT_GET(P0_05_PORT);
    if (!device_is_ready(gpio0_dev)) {
        LOG_ERR("GPIO0 device not ready");
        return -ENODEV;
    }
    
    // 使用底层GPIO配置，完全绕过设备树设置
    ret = gpio_pin_configure(gpio0_dev, P0_05_PIN, 
                           GPIO_INPUT | GPIO_PULL_UP | GPIO_ACTIVE_HIGH);
    if (ret < 0) {
        LOG_ERR("Failed to configure P0.05 as GPIO input: %d", ret);
        return ret;
    }
    
    LOG_INF("P0.05 successfully forced to GPIO input with pull-up");
    
    // 验证配置和初始状态
    int pin_state = gpio_pin_get(gpio0_dev, P0_05_PIN);
    LOG_INF("P0.05 initial state: %d", pin_state);
    
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
    LOG_INF("P0.05 status: device_ready=%d, pin_state=%d", 
           device_is_ready(gpio0_dev), state);
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
        
        // 记录成功重配次数
        if (reconfig_count % 10 == 0) {
            LOG_DBG("P0.05 reconfiguration successful, count: %d", reconfig_count);
        }
        
        // 检查引脚状态（调试）
        if (reconfig_count <= 3) {
            check_p0_05_status();
        }
    } else {
        LOG_ERR("P0.05 reconfiguration failed: %d", ret);
    }
    
    // 动态调整重配频率：
    // - 前5次：快速重配（100ms）
    // - 接下来10次：中等频率（500ms）  
    // - 之后：较低频率（2000ms）
    uint32_t delay_ms;
    if (reconfig_count < 5) {
        delay_ms = 100;
    } else if (reconfig_count < 15) {
        delay_ms = 500;
    } else {
        delay_ms = 2000;
    }
    
    k_work_reschedule(&reconfig_work, K_MSEC(delay_ms));
}

// 修正初始化函数 - 移除device参数
static int gpio_custom_init(void)
{
    LOG_INF("Initializing GPIO custom fix for P0.05");
    
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