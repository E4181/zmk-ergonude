/*
 * p0_05_enhance.h - P0.05 引脚增强头文件
 */

#ifndef P0_05_ENHANCE_H
#define P0_05_ENHANCE_H

#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 重新配置 P0.05 引脚
 * 用于需要重新初始化的情况
 */
void p0_05_reconfigure(void);

/**
 * @brief 读取 P0.05 引脚状态
 * @return 引脚状态 (0 = 低, 1 = 高)
 */
int p0_05_read_state(void);

/**
 * @brief 获取 P0.05 引脚的 PIN_CNF 配置值
 * @return PIN_CNF 寄存器值
 */
uint32_t p0_05_get_config(void);

#ifdef __cplusplus
}
#endif

#endif /* P0_05_ENHANCE_H */