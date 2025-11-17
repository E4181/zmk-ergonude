/*
 * gpio_custom.h - nRF52840 GPIO 自定义配置头文件
 */

#ifndef GPIO_CUSTOM_H
#define GPIO_CUSTOM_H

#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 读取矩阵行引脚当前状态
 * @return 引脚电平 (0 = 低电平, 1 = 高电平)
 */
int matrix_row_read_state(void);

/**
 * @brief 重新配置矩阵行引脚
 * 用于需要动态重新配置的场景
 */
void matrix_row_reconfigure(void);

/**
 * @brief 获取引脚当前的PIN_CNF配置值
 * @return PIN_CNF寄存器值
 */
uint32_t matrix_row_get_config(void);

/**
 * @brief 监控引脚状态（调试用）
 */
void matrix_row_monitor(void);

#ifdef __cplusplus
}
#endif

#endif /* GPIO_CUSTOM_H */