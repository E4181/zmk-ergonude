/* patch_pinmux.c - 放在你的 shield 目录下 */
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/drivers/gpio.h>

/* 重写原有的 pinmux_nrfmicro_init 函数 */
int pinmux_nrfmicro_init(void) {
    /* 完全跳过对 p0.05 的配置，或者按需配置 */
#if (CONFIG_BOARD_NRFMICRO_13 || CONFIG_BOARD_NRFMICRO_13_52833)
    const struct device *p0 = DEVICE_DT_GET(DT_NODELABEL(gpio0));
    
    /* 注释掉原有的充电器配置，改为输入模式 */
    // #if CONFIG_BOARD_NRFMICRO_CHARGER
    // gpio_pin_configure(p0, 5, GPIO_OUTPUT);
    // gpio_pin_set(p0, 5, 0);
    // #else
    gpio_pin_configure(p0, 5, GPIO_INPUT); // 或者完全跳过这行
    // #endif
#endif
    return 0;
}

/* 确保这个初始化替代原有的 */
SYS_INIT(pinmux_nrfmicro_init, PRE_KERNEL_1, CONFIG_PINMUX_INIT_PRIORITY);