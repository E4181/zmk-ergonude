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
static struct k_work_delayable debug_work;

// 增强的引脚配置检查
static void check_p0_05_configuration(void)
{
    if (!gpio0_dev || !device_is_ready(gpio0_dev)) {
        LOG_ERR("GPIO0 not ready for configuration check");
        return;
    }
    
    // 尝试读取引脚状态
    int current_state = gpio_pin_get(gpio0_dev, P0_05_PIN);
    
    // 测试引脚方向：尝试设置为输出并读取状态
    int ret = gpio_pin_configure(gpio0_dev, P0_05_PIN, GPIO_OUTPUT_LOW);
    if (ret == 0) {
        k_busy_wait(10); // 短暂延迟
        int output_state = gpio_pin_get(gpio0_dev, P0_05_PIN);
        
        LOG_WRN("P0.05 was configurable as OUTPUT, current state: %d->%d", 
                current_state, output_state);
        
        // 立即恢复为输入下拉
        gpio_pin_configure(gpio0_dev, P0_05_PIN, GPIO_INPUT | GPIO_PULL_DOWN);
    } else {
        LOG_INF("P0.05 cannot be configured as OUTPUT (may be locked)");
    }
    
    // 最终状态
    int final_state = gpio_pin_get(gpio0_dev, P0_05_PIN);
    LOG_INF("P0.05 final state: %d", final_state);
}

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
    
    // 先尝试禁用可能的外设
    // 对于nRF52840，P0.05可能被UART、SPI或其他外设占用
    
    // 使用底层GPIO配置，配置为输入带下拉
    ret = gpio_pin_configure(gpio0_dev, P0_05_PIN, 
                           GPIO_INPUT | GPIO_PULL_DOWN);
    if (ret < 0) {
        LOG_ERR("Failed to configure P0.05 as GPIO input with pull-down: %d", ret);
        
        // 尝试不带下拉
        ret = gpio_pin_configure(gpio0_dev, P0_05_PIN, GPIO_INPUT);
        if (ret < 0) {
            LOG_ERR("Failed to configure P0.05 as GPIO input even without pull-down: %d", ret);
            return ret;
        } else {
            LOG_WRN("P0.05 configured as input without pull-down");
        }
    } else {
        LOG_INF("P0.05 successfully forced to GPIO input with pull-down");
    }
    
    // 验证配置
    check_p0_05_configuration();
    
    return 0;
}

// 调试信息输出
static void debug_handler(struct k_work *work)
{
    if (!gpio0_dev || !device_is_ready(gpio0_dev)) {
        return;
    }
    
    static int debug_count = 0;
    int state = gpio_pin_get(gpio0_dev, P0_05_PIN);
    
    if (debug_count < 10) { // 只输出前10次调试信息
        LOG_DBG("P0.05 debug [%d]: state=%d", debug_count, state);
    }
    
    debug_count++;
    
    // 每5秒输出一次状态
    k_work_reschedule(&debug_work, K_MSEC(5000));
}

// 定期重新配置P0.05
static void periodic_reconfig_handler(struct k_work *work)
{
    static int reconfig_count = 0;
    int ret;
    
    // 强制重新配置
    ret = force_p0_05_gpio_config();
    
    if (ret == 0) {
        reconfig_count++;
        
        if (reconfig_count <= 3) {
            LOG_INF("P0.05 reconfiguration %d successful", reconfig_count);
        } else if (reconfig_count % 10 == 0) {
            LOG_DBG("P0.05 reconfiguration count: %d", reconfig_count);
        }
    } else {
        LOG_ERR("P0.05 reconfiguration failed: %d", ret);
    }
    
    // 动态调整重配频率
    uint32_t delay_ms = (reconfig_count < 10) ? 200 : 1000;
    k_work_reschedule(&reconfig_work, K_MSEC(delay_ms));
}

// 初始化函数
static int gpio_custom_init(void)
{
    LOG_INF("Initializing enhanced GPIO custom fix for P0.05");
    
    // 等待系统基本初始化完成
    k_msleep(100);
    
    // 立即应用P0.05修复
    int ret = force_p0_05_gpio_config();
    if (ret < 0) {
        LOG_ERR("Initial P0.05 configuration failed, will retry periodically");
    }
    
    // 启动定期重配工作
    k_work_init_delayable(&reconfig_work, periodic_reconfig_handler);
    k_work_reschedule(&reconfig_work, K_MSEC(200));
    
    // 启动调试工作
    k_work_init_delayable(&debug_work, debug_handler);
    k_work_reschedule(&debug_work, K_MSEC(1000));
    
    LOG_INF("Enhanced GPIO custom fix initialized");
    
    return 0;
}

// 导出引脚状态检查函数
void gpio_custom_check_p0_05(void)
{
    if (!gpio0_dev || !device_is_ready(gpio0_dev)) {
        LOG_ERR("GPIO0 not ready");
        return;
    }
    
    int state = gpio_pin_get(gpio0_dev, P0_05_PIN);
    LOG_INF("P0.05 current state: %d", state);
    
    check_p0_05_configuration();
}

// 手动重新配置函数
int gpio_custom_reconfig_p0_05(void)
{
    LOG_INF("Manual P0.05 reconfiguration requested");
    return force_p0_05_gpio_config();
}

// 使用POST_KERNEL优先级，确保在大多数驱动之后初始化
SYS_INIT(gpio_custom_init, POST_KERNEL, 90);