#ifndef __CHRDEV_H__
#define __CHRDEV_H__

#define DEVICE_NAME "mapleay-chrdev-device"
#define CLASS_NAME  "mapleay-chrdev-class"
#define MINOR_BASE  0     /* 次设备号起始编号为 0 */
#define MINOR_COUNT 1     /* 次设备号的数量为 1   */
#define BUF_SIZE    1024  /* 内核缓冲区大小       */

#define LEDON  1  /* 硬件LED灯 */
#define LEDOFF 0
#define INPUT_DEVICE_NAME_MAX 64 // 名称缓冲区大小

struct gpio_pin_t{
	int gpio;
	struct gpio_desc *gpiod;
	int flag;
	int irq;
	/* 引入输入子系统和中断顶半部、底半部 */
	struct input_dev   *input_dev;  
    struct timer_list  debounce_timer;
};

/* 字符设备的自定义私有数据结构 */
struct cdev_private_data_t {
    char   *buffer;         /* 内核缓冲区 */
    size_t buf_size;       /* 缓冲区大小: 写依据此变量  */
    size_t data_len;       /* 当前数据长度：读依据此变量 */
};

typedef struct chrdev_object {
    struct cdev   dev;
    struct class  *dev_class;
    struct device *dev_device;
    struct cdev_private_data_t dev_data;
    dev_t  dev_num;
    struct gpio_pin_t *gpio_keys; /* 应用于按键中断实验 */
	struct gpio_pin_t *gpio_leds; /* 应用于LED灯亮灭实验 */
}chrdev_t;

#endif