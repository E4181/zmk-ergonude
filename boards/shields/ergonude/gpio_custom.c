#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#define P0_05_PORT DT_NODELABEL(gpio0)
#define P0_05_PIN  5

static const struct device *gpio0_dev;
static struct k_work_delayable check_work;

// 强力重新配置P0.05，确保它是输入模式
static int force_p0_05_input_config(void)
{
    int ret;
    
    gpio0_dev = DEVICE_DT_GET(P0_05_PORT);
    if (!device_is_ready(gpio0_dev)) {
        LOG_ERR("GPIO0 device not ready");
        return -ENODEV;
    }
    
    LOG_WRN("Force reconfiguring P0.05 as INPUT with pull-down");
    
    // 首先尝试设置为输入下拉
    ret = gpio_pin_configure(gpio0_dev, P0_05_PIN, GPIO_INPUT | GPIO_PULL_DOWN);
    if (ret == 0) {
        LOG_INF("P0.05 successfully configured as input with pull-down");
        
        // 验证配置
        int state = gpio_pin_get(gpio0_dev, P0_05_PIN);
        LOG_INF("P0.05 state after config: %d", state);
        return 0;
    }
    
    LOG_ERR("Failed to configure P0.05 as input: %d", ret);
    return ret;
}

// 检查P0.05当前模式
static void check_p0_05_mode(void)
{
    if (!gpio0_dev || !device_is_ready(gpio0_dev)) {
        return;
    }
    
    // 通过尝试改变状态来检测当前模式
    // 移除未使用的变量 original_state
    
    // 尝试设置为输出低电平
    int ret = gpio_pin_configure(gpio0_dev, P0_05_PIN, GPIO_OUTPUT_LOW);
    if (ret == 0) {
        LOG_WRN("P0.05 was configurable as OUTPUT! It should be INPUT.");
        k_busy_wait(100);
        int new_state = gpio_pin_get(gpio0_dev, P0_05_PIN);
        LOG_WRN("P0.05 output low state: %d", new_state);
        
        // 立即恢复为输入
        gpio_pin_configure(gpio0_dev, P0_05_PIN, GPIO_INPUT | GPIO_PULL_DOWN);
    } else {
        LOG_INF("P0.05 appears to be locked as input (good)");
    }
    
    LOG_INF("P0.05 final check - state: %d", gpio_pin_get(gpio0_dev, P0_05_PIN));
}

// 定期检查工作处理函数
static void periodic_check_handler(struct k_work *work)
{
    check_p0_05_mode();
    k_work_reschedule(&check_work, K_MSEC(2000));
}

static int gpio_p0_05_fix_init(void)
{
    LOG_INF("Initializing P0.05 fix");
    k_msleep(200); // 等待系统稳定
    
    int ret = force_p0_05_input_config();
    if (ret == 0) {
        check_p0_05_mode();
    }
    
    // 设置定期检查
    k_work_init_delayable(&check_work, periodic_check_handler);
    k_work_reschedule(&check_work, K_MSEC(1000));
    
    return 0;
}

SYS_INIT(gpio_p0_05_fix_init, POST_KERNEL, 50);