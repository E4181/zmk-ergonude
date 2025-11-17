/*
 * p0_05_enhance.c - 只针对 P0.05 引脚进行增强配置
 * 其他行保持原有设备树配置
 */

#include <zephyr/kernel.h>
#include <zephyr/init.h>
#include <zephyr/sys/printk.h>

#define NRF_P0_BASE              0x50000000UL

/* GPIO 寄存器偏移量 */
#define GPIO_OUTSET_OFFSET       0x508
#define GPIO_OUTCLR_OFFSET       0x50C
#define GPIO_DIRSET_OFFSET       0x518
#define GPIO_DIRCLR_OFFSET       0x51C
#define GPIO_IN_OFFSET           0x510
#define GPIO_PIN_CNF_OFFSET      0x700

/* 只配置 P0.05 */
#define TARGET_ROW_PIN           5

/* 寄存器访问函数 */
static inline volatile uint32_t* gpio_reg(uint32_t offset)
{
    return (volatile uint32_t*)(NRF_P0_BASE + offset);
}

static inline volatile uint32_t* pin_cnf_reg(uint8_t pin)
{
    return gpio_reg(GPIO_PIN_CNF_OFFSET + (pin * 4));
}

/**
 * @brief 只增强 P0.05 的下拉驱动能力
 * 其他行保持设备树的下拉配置
 */
static void enhance_p0_05_pulldown_only(void)
{
    volatile uint32_t *pin_cnf = pin_cnf_reg(TARGET_ROW_PIN);
    
    printk("Enhancing ONLY P0.%02d pulldown strength...\n", TARGET_ROW_PIN);
    
    /* 读取原始配置 */
    uint32_t original = *pin_cnf;
    printk("P0.%02d original PIN_CNF: 0x%08X\n", TARGET_ROW_PIN, original);
    
    /* 配置为高驱动强度的下拉输入 */
    *pin_cnf = (0 << 0)  |  /* DIR = Input */
               (1 << 1)  |  /* INPUT = Connect */
               (1 << 2)  |  /* PULL = Pulldown */
               (6 << 8)  |  /* DRIVE = High drive (增强驱动能力) */
               (0 << 16);   /* SENSE = Disabled */
    
    /* 增强下拉效果 - 多次短暂输出低电平 */
    for (int i = 0; i < 3; i++) {
        *gpio_reg(GPIO_DIRSET_OFFSET) = (1UL << TARGET_ROW_PIN);
        *gpio_reg(GPIO_OUTCLR_OFFSET) = (1UL << TARGET_ROW_PIN);
        k_busy_wait(30);  /* 保持低电平30us */
        *gpio_reg(GPIO_DIRCLR_OFFSET) = (1UL << TARGET_ROW_PIN);
        k_busy_wait(10);  /* 间隔10us */
    }
    
    uint32_t final_config = *pin_cnf;
    uint32_t pin_state = (*gpio_reg(GPIO_IN_OFFSET) >> TARGET_ROW_PIN) & 1;
    
    printk("P0.%02d final PIN_CNF: 0x%08X\n", TARGET_ROW_PIN, final_config);
    printk("P0.%02d current state: %s\n", TARGET_ROW_PIN, pin_state ? "HIGH" : "LOW");
    
    /* 验证配置 */
    if (((final_config >> 2) & 0x3) == 1 && ((final_config >> 8) & 0xF) == 6) {
        printk("✓ P0.%02d successfully enhanced with strong pulldown\n", TARGET_ROW_PIN);
    } else {
        printk("✗ P0.%02d enhancement failed\n", TARGET_ROW_PIN);
    }
}

/**
 * @brief 测试 P0.05 单独的功能
 */
static void test_p0_05_functionality(void)
{
    printk("Testing P0.%02d functionality...\n", TARGET_ROW_PIN);
    
    /* 设置为输出低电平 */
    *gpio_reg(GPIO_DIRSET_OFFSET) = (1UL << TARGET_ROW_PIN);
    *gpio_reg(GPIO_OUTCLR_OFFSET) = (1UL << TARGET_ROW_PIN);
    k_busy_wait(100);
    
    uint32_t output_state = (*gpio_reg(GPIO_IN_OFFSET) >> TARGET_ROW_PIN) & 1;
    printk("P0.%02d forced LOW, measured: %s\n", 
           TARGET_ROW_PIN, output_state ? "HIGH" : "LOW");
    
    /* 恢复为输入 */
    *gpio_reg(GPIO_DIRCLR_OFFSET) = (1UL << TARGET_ROW_PIN);
    
    if (output_state) {
        printk("WARNING: P0.%02d may have hardware issue\n", TARGET_ROW_PIN);
    }
}

/**
 * @brief 初始化函数 - 只配置 P0.05
 */
static int p0_05_enhance_init(void)
{
    printk("\n========================================\n");
    printk("P0.05 Only Enhancement Initialization\n");
    printk("Other rows remain with device tree configuration\n");
    printk("========================================\n");
    
    /* 只增强 P0.05 的下拉能力 */
    enhance_p0_05_pulldown_only();
    
    /* 测试功能 */
    test_p0_05_functionality();
    
    printk("P0.05 enhancement completed\n");
    printk("========================================\n");
    
    return 0;
}

/* 在ZMK kscan初始化之前运行 */
SYS_INIT(p0_05_enhance_init, APPLICATION, 0);

/* API函数 - 只针对 P0.05 */
void p0_05_reconfigure(void)
{
    enhance_p0_05_pulldown_only();
}

int p0_05_read_state(void)
{
    return (*gpio_reg(GPIO_IN_OFFSET) >> TARGET_ROW_PIN) & 1;
}

uint32_t p0_05_get_config(void)
{
    return *pin_cnf_reg(TARGET_ROW_PIN);
}