// custom_gpio_init.c
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/init.h>

static int custom_gpio_init(void)
{
    const struct device *gpio0 = DEVICE_DT_GET(DT_NODELABEL(gpio0));
    
    if (!device_is_ready(gpio0)) {
        return -ENODEV;
    }
    
    // 强制配置P0.05为输入下拉
    int ret = gpio_pin_configure(gpio0, 5, GPIO_INPUT | GPIO_PULL_DOWN);
    if (ret < 0) {
        return ret;
    }
    
    return 0;
}

// 在系统启动时早期初始化
SYS_INIT(custom_gpio_init, POST_KERNEL, CONFIG_APPLICATION_INIT_PRIORITY);