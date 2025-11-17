#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#define P0_05_PORT DT_NODELABEL(gpio0)
#define P0_05_PIN  5

static const struct device *gpio0_dev;

// 尝试不同的配置来增强P0.05的下拉能力
static int enhance_p0_05_pull_strength(void)
{
    int ret;
    
    gpio0_dev = DEVICE_DT_GET(P0_05_PORT);
    if (!device_is_ready(gpio0_dev)) {
        LOG_ERR("GPIO0 device not ready");
        return -ENODEV;
    }
    
    LOG_WRN("Attempting to enhance P0.05 pull strength");
    
    // 方法1: 首先尝试标准输入下拉配置
    ret = gpio_pin_configure(gpio0_dev, P0_05_PIN, GPIO_INPUT | GPIO_PULL_DOWN);
    if (ret < 0) {
        LOG_ERR("Standard pull-down failed: %d", ret);
        return ret;
    }
    
    LOG_INF("P0.05 standard pull-down configured");
    
    // 验证配置
    int state = gpio_pin_get(gpio0_dev, P0_05_PIN);
    LOG_INF("P0.05 state after standard config: %d", state);
    
    return 0;
}

// 尝试使用nRF特定配置增强下拉
static int nrf_enhance_p0_05_pull(void)
{
    // 对于nRF52系列，我们可以尝试通过nrfx直接配置更强的下拉
    // 注意：这需要包含nRF特定的头文件
    
    #ifdef CONFIG_HAS_NRFX
    #include <nrfx_gpiote.h>
    #include <hal/nrf_gpio.h>
    
    // 使用nRF HAL直接配置引脚
    nrf_gpio_cfg_input(P0_05_PIN, NRF_GPIO_PIN_PULLDOWN);
    
    LOG_INF("P0.05 configured with nRF-specific pull-down");
    return 0;
    #else
    LOG_ERR("nRF specific enhancements not available");
    return -ENOTSUP;
    #endif
}

// 备用方案：尝试将P0.05配置为强推挽输出，在扫描时动态切换
static int configure_p0_05_as_strong_output(void)
{
    int ret;
    
    // 配置为强推挽输出，低电平
    ret = gpio_pin_configure(gpio0_dev, P0_05_PIN, 
                           GPIO_OUTPUT | GPIO_OUTPUT_INIT_LOW | GPIO_DRIVE_STRONG);
    if (ret < 0) {
        LOG_ERR("Failed to configure P0.05 as strong output: %d", ret);
        return ret;
    }
    
    LOG_WRN("P0.05 configured as STRONG OUTPUT (alternative approach)");
    return 0;
}

static int gpio_p0_05_fix_init(void)
{
    LOG_INF("Initializing P0.05 pull strength enhancement");
    
    // 等待系统稳定
    k_msleep(150);
    
    // 首先尝试标准方法
    int ret = enhance_p0_05_pull_strength();
    
    if (ret != 0) {
        LOG_ERR("Standard method failed, trying nRF-specific method");
        
        // 尝试nRF特定方法
        ret = nrf_enhance_p0_05_pull();
        
        if (ret != 0) {
            LOG_ERR("nRF method failed, trying output approach");
            
            // 最后尝试输出方法
            ret = configure_p0_05_as_strong_output();
        }
    }
    
    if (ret == 0) {
        LOG_INF("P0.05 configuration completed successfully");
    } else {
        LOG_ERR("All P0.05 configuration methods failed");
    }
    
    return ret;
}

SYS_INIT(gpio_p0_05_fix_init, POST_KERNEL, 70);