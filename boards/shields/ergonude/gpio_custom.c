#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/gpio/gpio_emul.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#define P0_05_PORT DT_NODELABEL(gpio0)
#define P0_05_PIN  5

static const struct device *gpio0_dev;

// 直接使用nRF寄存器操作配置P0.05
static void nrf_direct_configure_p0_05(void)
{
    // 包含nRF特定头文件
    #include <hal/nrf_gpio.h>
    
    LOG_INF("Using nRF direct register access for P0.05");
    
    // 直接配置P0.05为输入，带下拉
    nrf_gpio_cfg_input(P0_05_PIN, NRF_GPIO_PIN_PULLDOWN);
    
    LOG_INF("P0.05 directly configured as input with pulldown");
}

// 使用Zephyr GPIO API的强力配置
static int force_configure_p0_05(void)
{
    int ret;
    
    gpio0_dev = DEVICE_DT_GET(P0_05_PORT);
    if (!device_is_ready(gpio0_dev)) {
        LOG_ERR("GPIO0 device not ready");
        return -ENODEV;
    }
    
    // 首先尝试标准配置
    ret = gpio_pin_configure(gpio0_dev, P0_05_PIN, GPIO_INPUT | GPIO_PULL_DOWN);
    if (ret == 0) {
        LOG_INF("P0.05 standard configuration successful");
        return 0;
    }
    
    LOG_WRN("Standard configuration failed: %d", ret);
    return ret;
}

// 验证引脚配置
static void verify_p0_05_configuration(void)
{
    if (!gpio0_dev || !device_is_ready(gpio0_dev)) {
        return;
    }
    
    // 多次读取引脚状态以确认配置
    for (int i = 0; i < 3; i++) {
        int state = gpio_pin_get(gpio0_dev, P0_05_PIN);
        LOG_INF("P0.05 verification read %d: state=%d", i, state);
        k_busy_wait(1000); // 1ms延迟
    }
    
    // 测试引脚是否能被外部拉高（通过按键）
    LOG_INF("Please test P0.05 row keys now...");
}

// 强力的初始化函数
static int gpio_p0_05_force_init(void)
{
    LOG_INF("=== FORCE INITIALIZING P0.05 ===");
    
    // 延迟确保系统稳定
    k_msleep(200);
    
    // 方法1: 使用nRF直接寄存器访问
    nrf_direct_configure_p0_05();
    
    // 方法2: 使用Zephyr API作为备用
    int ret = force_configure_p0_05();
    if (ret != 0) {
        LOG_ERR("Both configuration methods may have failed");
    }
    
    // 验证配置
    k_msleep(100);
    verify_p0_05_configuration();
    
    LOG_INF("=== P0.05 FORCE INITIALIZATION COMPLETE ===");
    
    return 0;
}

// 使用较高的优先级确保早期执行
SYS_INIT(gpio_p0_05_force_init, POST_KERNEL, 10);