#include <zephyr/kernel.h>      // 用于 SYS_INIT
#include <zephyr/drivers/gpio.h> // GPIO API
#include <zephyr/logging/log.h>  // 日志（可选）

LOG_MODULE_REGISTER(my_gpio_init, LOG_LEVEL_DBG);  // 启用调试日志

#define GPIO_PORT "GPIO_0"  // nRF52840 的 P0.xx 属于 GPIO_0
#define PIN_NUMBER 5       // P0.05

// 初始化函数：在 boot 时配置 P0.05 为输入带下拉
static int init_p005_gpio(void) {

    const struct device *gpio_dev = device_get_binding(GPIO_PORT);
    if (!gpio_dev) {
        LOG_ERR("Failed to get GPIO_0 device");
        return -ENODEV;
    }

    // 配置为输入模式，带下拉电阻，高电平有效
    int ret = gpio_pin_configure(gpio_dev, PIN_NUMBER, GPIO_INPUT | GPIO_PULL_DOWN | GPIO_ACTIVE_HIGH);
    if (ret != 0) {
        LOG_ERR("Failed to configure P0.05 as input: %d", ret);
        return ret;
    }

    LOG_INF("P0.05 configured as GPIO input with pull-down");
    return 0;
}

// 在应用初始化优先级注册此函数（在 kscan 初始化前执行）
SYS_INIT(init_p005_gpio, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);