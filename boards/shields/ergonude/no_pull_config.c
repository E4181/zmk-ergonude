/* no_pull_config.c */
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/printk.h>

static int configure_p0_05_no_pull(void) {
    const struct device *gpio0 = DEVICE_DT_GET(DT_NODELABEL(gpio0));
    
    // 完全禁用内部上下拉电阻
    int ret = gpio_pin_configure(gpio0, 5, GPIO_INPUT);
    
    if (ret == 0) {
        printk("P0.05 configured as INPUT (no pull)\n");
        
        // 验证配置
        int state = gpio_pin_get(gpio0, 5);
        printk("Initial state: %d\n", state);
    }
    
    return ret;
}

SYS_INIT(configure_p0_05_no_pull, APPLICATION, 90);