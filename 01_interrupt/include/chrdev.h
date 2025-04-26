#ifndef __CHRDEV_H__
#define __CHRDEV_H__

#define DEVICE_NAME "mapleay-chrdev-device"
#define CLASS_NAME  "mapleay-chrdev-class"
#define MINOR_BASE  0     /* 次设备号起始编号为 0 */
#define MINOR_COUNT 1     /* 次设备号的数量为 1   */
#define BUF_SIZE    1024  /* 内核缓冲区大小       */

#define LEDON  1  /* 硬件LED灯 */
#define LEDOFF 0

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

    struct gpio_desc *led_gpio;    /* GPIO描述符 */ 
    //struct pinctrl   *pinctrl;   /* 无需 pinctrl */ 
}chrdev_t;

#endif