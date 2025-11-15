/* 自定义引脚配置文件 - custom_pinmux.c */
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/devicetree.h>
#include <zephyr/sys/printk.h>

static int custom_pinmux_init(void) {
    const struct device *gpio0 = DEVICE_DT_GET(DT_NODELABEL(gpio0));
    
    if (!device_is_ready(gpio0)) {
        printk("GPIO0 device not ready\n");
        return -ENODEV;
    }
    
    /* 等待原初始化完成 */
    k_msleep(5);
    
    /* 重新配置 P0.05 为矩阵行输入引脚 */
    int ret = gpio_pin_configure(gpio0, 5, GPIO_INPUT | GPIO_PULL_DOWN);
    
    if (ret == 0) {
        printk("Successfully configured P0.05 as matrix row input\n");
    } else {
        printk("Failed to configure P0.05: %d\n", ret);
    }
    
    return ret;
}

/* 使用 POST_KERNEL 和较高优先级确保覆盖 */
SYS_INIT(custom_pinmux_init, POST_KERNEL, 80);