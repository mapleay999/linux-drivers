#ifndef __STM32MP157d_H__
#define __STM32MP157d_H__

#define LEDOFF     0  /* 关灯 */
#define LEDON      1  /* 开灯 */
#define LEDPIN_NO  11 /* 控制开关灯的pin引脚的逻辑编号，自定义的，用于平台设备驱动框架的硬件数据获取后的核对 */

// /* 寄存器物理地址 */ 
// #define PERIPH_BASE              (0x40000000)
// #define MPU_AHB4_PERIPH_BASE     (PERIPH_BASE + 0x10000000)
// #define RCC_BASE                 (MPU_AHB4_PERIPH_BASE + 0x0000)
// #define RCC_MP_AHB4ENSETR        (RCC_BASE + 0XA28)
// #define GPIOI_BASE               (MPU_AHB4_PERIPH_BASE + 0xA000)
// #define GPIOI_MODER              (GPIOI_BASE + 0x0000)
// #define GPIOI_OTYPER             (GPIOI_BASE + 0x0004)
// #define GPIOI_OSPEEDR            (GPIOI_BASE + 0x0008)
// #define GPIOI_PUPDR              (GPIOI_BASE + 0x000C)
// #define GPIOI_BSRR               (GPIOI_BASE + 0x0018)

// void led_init(void); //需写出，否则其他c文件调用，提示非显性警告。
// void led_switch(u8 sta);
// void led_deinit(void);

#endif