#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#define P0_05_PORT DT_NODELABEL(gpio0)
#define P0_05_PIN  5

static const struct device *gpio0_dev;
static struct k_work_delayable hammer_work;

// 强力重置P0.05配置
static void hammer_p0_05_config(void)
{
    static int hammer_count = 0;
    
    if (!gpio0_dev) {
        gpio0_dev = DEVICE_DT_GET(P0_05_PORT);
    }
    
    if (!device_is_ready(gpio0_dev)) {
        if (hammer_count < 3) {
            LOG_ERR("GPIO0 not ready");
        }
        return;
    }
    
    // 方法1: 先尝试配置为输出低电平，确保我们能控制引脚
    int ret = gpio_pin_configure(gpio0_dev, P0_05_PIN, GPIO_OUTPUT | GPIO_OUTPUT_INIT_LOW);
    if (ret == 0) {
        // 短暂保持输出低电平
        k_busy_wait(100);
        
        // 然后立即重新配置为输入下拉
        ret = gpio_pin_configure(gpio0_dev, P0_05_PIN, GPIO_INPUT | GPIO_PULL_DOWN);
        
        if (ret == 0) {
            hammer_count++;
            
            if (hammer_count <= 5 || (hammer_count % 20 == 0)) {
                LOG_INF("P0.05 hammer config #%d: OUTPUT->INPUT", hammer_count);
                
                // 读取状态验证
                int state = gpio_pin_get(gpio0_dev, P0_05_PIN);
                LOG_INF("P0.05 state after hammer: %d", state);
            }
        } else {
            LOG_ERR("P0.05 input reconfiguration failed: %d", ret);
        }
    } else {
        LOG_ERR("P0.05 output configuration failed: %d", ret);
    }
}

// 尝试不同的配置方法
static void alternative_p0_05_config(void)
{
    static int alt_count = 0;
    
    if (!gpio0_dev || !device_is_ready(gpio0_dev)) {
        return;
    }
    
    // 尝试不带上下拉的配置
    int ret = gpio_pin_configure(gpio0_dev, P0_05_PIN, GPIO_INPUT);
    if (ret == 0) {
        alt_count++;
        if (alt_count <= 3) {
            LOG_INF("P0.05 alternative config #%d: input without pull", alt_count);
        }
    }
}

static void hammer_handler(struct k_work *work)
{
    static int work_count = 0;
    
    // 主要使用强力重置方法
    hammer_p0_05_config();
    
    // 偶尔尝试替代方法
    if (work_count % 10 == 0) {
        alternative_p0_05_config();
    }
    
    work_count++;
    
    // 非常激进的频率：前期快速，后期维持
    uint32_t delay_ms = (work_count < 20) ? 50 :   // 50ms间隔
                        (work_count < 50) ? 100 :  // 100ms间隔
                        500;                       // 500ms间隔
    
    k_work_reschedule(&hammer_work, K_MSEC(delay_ms));
}

static int gpio_p0_05_hammer_init(void)
{
    LOG_INF("=== STARTING P0.05 HAMMER FIX ===");
    
    // 获取GPIO设备
    gpio0_dev = DEVICE_DT_GET(P0_05_PORT);
    if (!device_is_ready(gpio0_dev)) {
        LOG_ERR("GPIO0 not ready at init");
        // 我们仍然会尝试，因为设备可能在之后变得可用
    }
    
    // 立即开始强力修复
    hammer_p0_05_config();
    
    // 启动持续修复
    k_work_init_delayable(&hammer_work, hammer_handler);
    k_work_reschedule(&hammer_work, K_MSEC(10)); // 10ms后开始
    
    LOG_INF("=== P0.05 HAMMER FIX STARTED ===");
    return 0;
}

SYS_INIT(gpio_p0_05_hammer_init, POST_KERNEL, 30);