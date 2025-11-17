/*
 * gpio_custom.h - nRF52840 P0.05 引脚自定义配置头文件
 */

#ifndef GPIO_CUSTOM_H
#define GPIO_CUSTOM_H

#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 读取 P0.05 矩阵行引脚当前状态
 * @return 引脚电平 (0 = 低电平, 1 = 高电平)
 */
int matrix_row_p0_05_read_state(void);

/**
 * @brief 重新配置 P0.05 引脚
 * 完全覆盖所有现有配置
 */
void matrix_row_p0_05_reconfigure(void);

/**
 * @brief 获取 P0.05 引脚当前的 PIN_CNF 配置值
 * @return PIN_CNF 寄存器值
 */
uint32_t matrix_row_p0_05_get_config(void);

#ifdef __cplusplus
}
#endif

#endif /* GPIO_CUSTOM_H */