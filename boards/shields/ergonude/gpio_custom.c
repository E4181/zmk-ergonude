/*
 * gpio_custom.c - nRF52840 GPIO 底层寄存器配置
 * 用于ZMK键盘矩阵行引脚配置，加强下拉能力
 */

#include <zephyr/kernel.h>
#include <zephyr/types.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/init.h>
#include <zephyr/sys/printk.h>

/* nRF52840 GPIO 寄存器基地址 */
#define NRF_P0_BASE              0x50000000UL
#define NRF_GPIO_BASE(port)      (NRF_P0_BASE + 0x700UL)

/* GPIO 寄存器偏移量 */
#define GPIO_OUT_OFFSET          0x504
#define GPIO_OUTSET_OFFSET       0x508
#define GPIO_OUTCLR_OFFSET       0x50C
#define GPIO_IN_OFFSET           0x510
#define GPIO_DIR_OFFSET          0x514
#define GPIO_DIRSET_OFFSET       0x518
#define GPIO_DIRCLR_OFFSET       0x51C
#define GPIO_LATCH_OFFSET        0x520
#define GPIO_DETECTMODE_OFFSET   0x524
#define GPIO_PIN_CNF_OFFSET(n)   (0x700 + ((n) * 4))

/* 寄存器访问宏 */
#define NRF_GPIO_REG(port, offset) (*(volatile uint32_t *)(NRF_GPIO_BASE(port) + (offset)))

/* 矩阵行引脚定义 */
#define MATRIX_ROW_PIN           5   /* P0.05 */

/* PIN_CNF 寄存器位定义 */
#define GPIO_PIN_CNF_DIR_Pos     0
#define GPIO_PIN_CNF_DIR_Msk     (1UL << GPIO_PIN_CNF_DIR_Pos)
#define GPIO_PIN_CNF_INPUT_Pos   1
#define GPIO_PIN_CNF_INPUT_Msk   (1UL << GPIO_PIN_CNF_INPUT_Pos)
#define GPIO_PIN_CNF_PULL_Pos    2
#define GPIO_PIN_CNF_PULL_Msk    (3UL << GPIO_PIN_CNF_PULL_Pos)
#define GPIO_PIN_CNF_DRIVE_Pos   8
#define GPIO_PIN_CNF_DRIVE_Msk   (0xFUL << GPIO_PIN_CNF_DRIVE_Pos)
#define GPIO_PIN_CNF_SENSE_Pos   16
#define GPIO_PIN_CNF_SENSE_Msk   (3UL << GPIO_PIN_CNF_SENSE_Pos)

/* 下拉电阻配置值 */
#define GPIO_PULL_DISABLED       0
#define GPIO_PULL_DOWN           1
#define GPIO_PULL_UP             3

/**
 * @brief 读取 P0.05 引脚的当前 PIN_CNF 配置
 * @return 当前的 PIN_CNF 寄存器值
 */
static uint32_t read_pin_configuration(void)
{
    return NRF_GPIO_REG(0, GPIO_PIN_CNF_OFFSET(MATRIX_ROW_PIN));
}

/**
 * @brief 打印引脚配置信息用于调试
 */
static void print_pin_configuration(const char *label, uint32_t config)
{
    printk("%s - PIN_CNF[%d]: 0x%08lX\n", label, MATRIX_ROW_PIN, config);
    printk("  DIR: %s, INPUT: %s, PULL: %s\n",
           (config & GPIO_PIN_CNF_DIR_Msk) ? "Output" : "Input",
           (config & GPIO_PIN_CNF_INPUT_Msk) ? "Connected" : "Disconnected",
           ((config >> GPIO_PIN_CNF_PULL_Pos) & 0x3) == GPIO_PULL_DOWN ? "Pull-down" :
           ((config >> GPIO_PIN_CNF_PULL_Pos) & 0x3) == GPIO_PULL_UP ? "Pull-up" : "Disabled");
    printk("  DRIVE: 0x%01lX, SENSE: 0x%01lX\n",
           (config >> GPIO_PIN_CNF_DRIVE_Pos) & 0xF,
           (config >> GPIO_PIN_CNF_SENSE_Pos) & 0x3);
}

/**
 * @brief 读取引脚当前输入状态
 * @return 引脚电平状态 (0 或 1)
 */
static uint32_t read_pin_state(void)
{
    return (NRF_GPIO_REG(0, GPIO_IN_OFFSET) >> MATRIX_ROW_PIN) & 1;
}

/**
 * @brief 增强下拉能力的配置方案
 * 通过软件方式短暂输出低电平来加强下拉效果
 */
static void enhance_pulldown_strength(void)
{
    volatile uint32_t *dirset = &NRF_GPIO_REG(0, GPIO_DIRSET_OFFSET);
    volatile uint32_t *dirclr = &NRF_GPIO_REG(0, GPIO_DIRCLR_OFFSET);
    volatile uint32_t *outclr = &NRF_GPIO_REG(0, GPIO_OUTCLR_OFFSET);
    
    /* 短暂配置为输出低电平 */
    *dirset = (1UL << MATRIX_ROW_PIN);   /* 设置为输出模式 */
    *outclr = (1UL << MATRIX_ROW_PIN);   /* 输出低电平 */
    
    /* 保持低电平输出一段时间（约10us） */
    k_busy_wait(10);
    
    /* 恢复为输入模式，此时引脚被强拉到低电平 */
    *dirclr = (1UL << MATRIX_ROW_PIN);
    
    printk("Enhanced pulldown strength for P0.%02d\n", MATRIX_ROW_PIN);
}

/**
 * @brief 配置 P0.05 为矩阵行引脚并加强下拉能力
 * 
 * 从最底层配置寄存器，确保引脚为：
 * - 输入模式
 * - 启用输入缓冲器
 * - 强下拉电阻
 * - 标准驱动强度
 * - 禁用感应功能
 */
static void configure_matrix_row_pin(void)
{
    volatile uint32_t *pin_cnf = &NRF_GPIO_REG(0, GPIO_PIN_CNF_OFFSET(MATRIX_ROW_PIN));
    volatile uint32_t *dirclr = &NRF_GPIO_REG(0, GPIO_DIRCLR_OFFSET);
    
    uint32_t original_config = read_pin_configuration();
    
    printk("\n=== Configuring P0.%02d as Matrix Row Pin ===\n", MATRIX_ROW_PIN);
    print_pin_configuration("Original configuration", original_config);
    
    /* 第一步：确保引脚为输入模式 */
    *dirclr = (1UL << MATRIX_ROW_PIN);
    
    /* 第二步：配置 PIN_CNF 寄存器 */
    uint32_t new_config = 0;
    
    /* DIR = 0 (输入模式) */
    new_config &= ~GPIO_PIN_CNF_DIR_Msk;
    
    /* INPUT = 1 (连接输入缓冲器) */
    new_config |= GPIO_PIN_CNF_INPUT_Msk;
    
    /* PULL = 0b01 (下拉电阻) */
    new_config &= ~GPIO_PIN_CNF_PULL_Msk;
    new_config |= (GPIO_PULL_DOWN << GPIO_PIN_CNF_PULL_Pos);
    
    /* DRIVE = 0b0000 (S0S1 标准驱动) */
    new_config &= ~GPIO_PIN_CNF_DRIVE_Msk;
    
    /* SENSE = 0b00 (禁用感应) */
    new_config &= ~GPIO_PIN_CNF_SENSE_Msk;
    
    /* 写入新的配置 */
    *pin_cnf = new_config;
    
    /* 第三步：增强下拉能力 */
    enhance_pulldown_strength();
    
    /* 验证最终配置 */
    uint32_t final_config = read_pin_configuration();
    print_pin_configuration("Final configuration", final_config);
    
    /* 读取当前引脚状态 */
    uint32_t pin_state = read_pin_state();
    printk("Current pin state: %s\n", pin_state ? "HIGH" : "LOW");
    
    /* 配置验证 */
    if ((final_config & GPIO_PIN_CNF_INPUT_Msk) && 
        ((final_config >> GPIO_PIN_CNF_PULL_Pos) & 0x3) == GPIO_PULL_DOWN) {
        printk("✓ P0.%02d successfully configured as matrix row with enhanced pulldown\n", 
               MATRIX_ROW_PIN);
    } else {
        printk("✗ P0.%02d configuration failed!\n", MATRIX_ROW_PIN);
    }
}

/**
 * @brief 监控引脚状态的调试函数
 */
static void monitor_pin_status(void)
{
    uint32_t pin_state = read_pin_state();
    uint32_t pin_config = read_pin_configuration();
    
    printk("P0.%02d Monitor - State: %d, Config: 0x%08lX\n", 
           MATRIX_ROW_PIN, pin_state, pin_config);
}

/**
 * @brief 初始化函数 - 在系统启动时调用
 */
static int gpio_custom_init(const struct device *dev)
{
    ARG_UNUSED(dev);
    
    printk("\n");
    printk("========================================\n");
    printk("nRF52840 GPIO Custom Configuration\n");
    printk("Matrix Row Pin: P0.%02d\n", MATRIX_ROW_PIN);
    printk("========================================\n");
    
    /* 配置矩阵行引脚 */
    configure_matrix_row_pin();
    
    /* 初始状态监控 */
    monitor_pin_status();
    
    printk("GPIO custom configuration completed\n");
    printk("========================================\n");
    
    return 0;
}

/* 注册初始化函数，在应用程序初始化阶段调用 */
SYS_INIT(gpio_custom_init, APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);

/**
 * @brief 提供给其他模块使用的API函数
 */

/* 读取矩阵行引脚状态 */
int matrix_row_read_state(void)
{
    return (int)read_pin_state();
}

/* 重新配置引脚（如果需要动态调整） */
void matrix_row_reconfigure(void)
{
    configure_matrix_row_pin();
}

/* 获取引脚配置信息 */
uint32_t matrix_row_get_config(void)
{
    return read_pin_configuration();
}

/* 引脚状态监控（用于调试） */
void matrix_row_monitor(void)
{
    monitor_pin_status();
}