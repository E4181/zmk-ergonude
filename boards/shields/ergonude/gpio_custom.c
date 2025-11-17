/*
 * gpio_custom.c - nRF52840 P0.05 引脚底层寄存器配置
 * 完全覆盖板级预设，配置为矩阵行引脚并增强下拉能力
 */

#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/sys/printk.h>

/* nRF52840 寄存器定义 - 使用直接地址访问 */
#define NRF_P0_BASE              0x50000000UL

/* GPIO 寄存器偏移量 */
#define GPIO_OUT_OFFSET          0x504
#define GPIO_OUTSET_OFFSET       0x508
#define GPIO_OUTCLR_OFFSET       0x50C
#define GPIO_IN_OFFSET           0x510
#define GPIO_DIR_OFFSET          0x514
#define GPIO_DIRSET_OFFSET       0x518
#define GPIO_DIRCLR_OFFSET       0x51C
#define GPIO_PIN_CNF_OFFSET      0x700

/* 目标引脚 */
#define MATRIX_ROW_PIN           5

/* 寄存器访问函数 */
static inline volatile uint32_t* get_gpio_reg_ptr(uint32_t offset)
{
    return (volatile uint32_t*)(NRF_P0_BASE + offset);
}

static inline uint32_t read_gpio_reg(uint32_t offset)
{
    return *get_gpio_reg_ptr(offset);
}

static inline void write_gpio_reg(uint32_t offset, uint32_t value)
{
    *get_gpio_reg_ptr(offset) = value;
}

static inline volatile uint32_t* get_pin_cnf_ptr(uint8_t pin)
{
    return get_gpio_reg_ptr(GPIO_PIN_CNF_OFFSET + (pin * 4));
}

static inline uint32_t read_pin_cnf(uint8_t pin)
{
    return *get_pin_cnf_ptr(pin);
}

static inline void write_pin_cnf(uint8_t pin, uint32_t value)
{
    *get_pin_cnf_ptr(pin) = value;
}

/**
 * @brief 完全覆盖 P0.05 引脚的配置
 * 
 * 此函数将：
 * 1. 清除所有现有配置
 * 2. 配置为输入模式
 * 3. 启用输入缓冲器
 * 4. 启用强下拉电阻
 * 5. 增强下拉能力
 */
static void override_p0_05_configuration(void)
{
    uint32_t original_config;
    uint32_t new_config;
    
    printk("=== Overriding P0.%02d configuration ===\n", MATRIX_ROW_PIN);
    
    /* 读取原始配置 */
    original_config = read_pin_cnf(MATRIX_ROW_PIN);
    printk("Original PIN_CNF: 0x%08X\n", original_config);
    
    /* 第一步：确保引脚为输入模式 */
    write_gpio_reg(GPIO_DIRCLR_OFFSET, (1UL << MATRIX_ROW_PIN));
    
    /* 第二步：彻底重置并重新配置 PIN_CNF 寄存器 */
    new_config = 0;
    
    /* BIT 0: DIR = 0 (输入模式) */
    new_config &= ~(1UL << 0);
    
    /* BIT 1: INPUT = 1 (连接输入缓冲器) */
    new_config |= (1UL << 1);
    
    /* BIT 2-3: PULL = 0b01 (下拉电阻) */
    new_config &= ~(3UL << 2);  /* 清除原有配置 */
    new_config |= (1UL << 2);   /* 设置为下拉 */
    
    /* BIT 8-11: DRIVE = 0b0000 (S0S1 标准驱动) */
    new_config &= ~(0xFUL << 8);
    
    /* BIT 16-17: SENSE = 0b00 (禁用感应) */
    new_config &= ~(3UL << 16);
    
    /* 写入新的配置 */
    write_pin_cnf(MATRIX_ROW_PIN, new_config);
    
    /* 第三步：增强下拉能力 - 通过短暂输出低电平 */
    write_gpio_reg(GPIO_DIRSET_OFFSET, (1UL << MATRIX_ROW_PIN));  /* 设置为输出 */
    write_gpio_reg(GPIO_OUTCLR_OFFSET, (1UL << MATRIX_ROW_PIN));  /* 输出低电平 */
    k_busy_wait(20);  /* 保持20us，确保电容放电 */
    write_gpio_reg(GPIO_DIRCLR_OFFSET, (1UL << MATRIX_ROW_PIN));  /* 恢复为输入 */
    
    /* 验证最终配置 */
    uint32_t final_config = read_pin_cnf(MATRIX_ROW_PIN);
    uint32_t pin_state = (read_gpio_reg(GPIO_IN_OFFSET) >> MATRIX_ROW_PIN) & 1;
    
    printk("Final PIN_CNF: 0x%08X\n", final_config);
    printk("Pin state: %s\n", pin_state ? "HIGH" : "LOW");
    
    /* 详细配置解析 */
    printk("Configuration details:\n");
    printk("  DIR: %s\n", (final_config & (1 << 0)) ? "Output" : "Input");
    printk("  INPUT: %s\n", (final_config & (1 << 1)) ? "Connected" : "Disconnected");
    printk("  PULL: %s\n", ((final_config >> 2) & 0x3) == 1 ? "Pull-down" : 
                          ((final_config >> 2) & 0x3) == 3 ? "Pull-up" : "None");
    printk("  DRIVE: 0x%X\n", (uint32_t)((final_config >> 8) & 0xF));
    printk("  SENSE: 0x%X\n", (uint32_t)((final_config >> 16) & 0x3));
    
    if (((final_config >> 2) & 0x3) == 1) {  /* 检查PULL配置 */
        printk("✓ P0.%02d successfully configured as matrix row with enhanced pulldown\n", 
               MATRIX_ROW_PIN);
    } else {
        printk("✗ P0.%02d configuration failed - pull resistor not set correctly\n", 
               MATRIX_ROW_PIN);
    }
}

/**
 * @brief 监控引脚状态
 */
static void monitor_pin_status(void)
{
    uint32_t pin_state = (read_gpio_reg(GPIO_IN_OFFSET) >> MATRIX_ROW_PIN) & 1;
    uint32_t pin_config = read_pin_cnf(MATRIX_ROW_PIN);
    
    printk("P0.%02d Status - State: %d, Config: 0x%08X\n", 
           MATRIX_ROW_PIN, pin_state, pin_config);
}

/**
 * @brief 初始化函数
 */
static int gpio_custom_init(void)
{
    printk("\n");
    printk("========================================\n");
    printk("nRF52840 P0.05 Complete Register Override\n");
    printk("Configuring as Matrix Row with Enhanced Pulldown\n");
    printk("========================================\n");
    
    /* 完全覆盖P0.05配置 */
    override_p0_05_configuration();
    
    /* 验证配置 */
    monitor_pin_status();
    
    printk("P0.05 configuration override completed\n");
    printk("========================================\n");
    
    return 0;
}

/* 在应用程序初始化阶段调用，确保覆盖所有预设配置 */
SYS_INIT(gpio_custom_init, APPLICATION, 50);

/* 提供给其他模块使用的API */
int matrix_row_p0_05_read_state(void)
{
    return (int)((read_gpio_reg(GPIO_IN_OFFSET) >> MATRIX_ROW_PIN) & 1);
}

void matrix_row_p0_05_reconfigure(void)
{
    override_p0_05_configuration();
}

uint32_t matrix_row_p0_05_get_config(void)
{
    return read_pin_cnf(MATRIX_ROW_PIN);
}