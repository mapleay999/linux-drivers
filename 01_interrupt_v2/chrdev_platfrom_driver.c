/* UTF-8编码 Unix(LF) */
#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/uaccess.h>  /* 用户空间数据拷贝 */
#include <linux/slab.h>     /* 内核内存分配    */
#include <linux/ioctl.h>    /* ioctl相关定义   */
#include <linux/string.h>   /* 内核的 strlen() */
#include <linux/platform_device.h>
#include <linux/types.h>
#include <linux/string.h>  /* 用内核自己的strcmp函数 */
#include "chrdev_ioctl.h"
#include "chrdev.h"         /* 自定义内核字符设备驱动框架信息 */
#include <linux/mod_devicetable.h> /* struct platform_device_id */
#include <linux/io.h>
#include <linux/of.h>          /* device-tree */
#include <linux/of_address.h>  /* device-tree */
#include <linux/gpio/consumer.h>    /* Linux的GPIO子系统头文件 */
#include <linux/pinctrl/consumer.h> /* Linux的PINCTRL子系统头文件 */
#include <linux/interrupt.h>
#include <linux/of_irq.h>
#include <linux/of_gpio.h>   /* of_gpio_count() */
#include <linux/timer.h>    /* 定时器核心头文件 */
#include <linux/input.h>    /* 输入子系统核心头文件 */

static chrdev_t chrdev; /* 自定义的设备对象结构体：面相对象思想。 */


static int dev_open(struct inode *inode, struct file *filp) {
    filp->private_data = &chrdev.dev_data; /* 初始化进程私有数据，挂接到设备驱动的私有数据空间 */
    printk(KERN_INFO "内核 chrdev_open：设备已被 pid %d 打开！\n", current->pid);
    return 0;
}

/* llseek 函数跟字符设备驱动的数据，可以无任何直接关联，只通过 offset 间接关联。 */
/* loff_t 类型是 signed long long 有符号数值 */
loff_t dev_llseek (struct file *filp, loff_t offset, int whence){

    struct cdev_private_data_t *data = filp->private_data;
    loff_t new_pos = 0;
    /* 对进程的 f_ops 进行校验，防止意外 */
    if ((filp->f_pos < 0) || (filp->f_pos > data->buf_size)) {
        return -EINVAL;
    }

    switch (whence) {
        case SEEK_SET:
                if ((offset < 0) || (offset > data->buf_size)) {
                    printk(KERN_INFO "内核 dev_llseek:SEEK_SET:文件偏移失败！！\n");
                    return -EINVAL;
                }
                filp->f_pos = offset;
                break;

        case SEEK_CUR:
                new_pos = filp->f_pos + offset;
                if ((new_pos < 0) || (new_pos > data->buf_size)){
                    printk(KERN_INFO "内核 dev_llseek:SEEK_CUR:文件偏移失败！！\n");
                    return -EINVAL;
                }
                filp->f_pos = new_pos;
                break;

        case SEEK_END:
                new_pos = data->buf_size + offset;
                if ((new_pos < 0) || (new_pos > data->buf_size)){
                    printk(KERN_INFO "内核 dev_llseek:SEEK_END:文件偏移失败！！\n");
                    return -EINVAL;
                }
                filp->f_pos = new_pos;
                break;

        default: return -EINVAL; /* 禁止隐式偏移 */
    }
    return filp->f_pos;
}

/* 提示：read/write处理风格都是：二进制安全型！所以使用char类型代表单个字节，所有以单个字节的操作都是安全且兼容性强的 */
static ssize_t dev_read(struct file *filp, char __user *buf, size_t len_to_meet, loff_t *off) {

    struct cdev_private_data_t *data = filp->private_data;
    size_t cnt_read = min_t(size_t, len_to_meet, data->data_len - *off); //min截短

    if (cnt_read == 0) {
        printk(KERN_INFO "内核 chrdev_read：内核数据早已读出完毕！无法继续读出！\n");
        return 0;
    }

    if (copy_to_user(buf, data->buffer + *off, cnt_read)) {
        printk(KERN_ERR "内核 chrdev_read：从内核复制数据到用户空间操作失败！\n");
        return -EFAULT;
    }

    *off += cnt_read;
    printk(KERN_INFO "内核 chrdev_read：已从内核读出 %zu 字节的数据，当下偏移位于 %lld处。\n", cnt_read, *off);
    return cnt_read;
}

/* 提示：read/write处理风格都是：二进制安全型！所以使用char类型代表单个字节，所有以单个字节的操作都是安全且兼容性强的 */
static ssize_t dev_write(struct file *filp, const char __user *buf, size_t len_to_meet, loff_t *off) {
    struct cdev_private_data_t *data = filp->private_data;
    size_t cnt_write = min_t(size_t, len_to_meet, data->buf_size - *off); //min，二进制安全，取小。OK。
    
    if (cnt_write == 0) {
        printk(KERN_INFO "内核 chrdev_write：内核缓冲区已满，无法继续写入！\n");
        return -ENOSPC;
    }
    
    if (copy_from_user(data->buffer + *off, buf, cnt_write)) {
        printk(KERN_ERR "内核 chrdev_write：从用户空间复制数据到内核空间的操作失败！\n");
        return -EFAULT;
    }

    /* 硬件LED灯控制部分: 提示，注意 data->buffer[0] 表示缓冲区第0位。而不是 (*off) */
    if(data->buffer[0] == LEDON) {
        gpiod_set_value(chrdev.gpio_leds->gpiod, 1);  /* 打开 LED 灯  */
    }
    else if(data->buffer[0] == LEDOFF) { 
        gpiod_set_value(chrdev.gpio_leds->gpiod, 0);  /* 关闭 LED 灯  */
    }
    else{
        printk(KERN_INFO "内核缓冲区内容：data->buffer[0]的值是：%d\n", data->buffer[0]);
        printk(KERN_ERR "硬件操控失败！\n");
    }

    *off += cnt_write;
    data->data_len = max_t(size_t, data->data_len, *off); //max，二进制安全，取大。OK。
    printk(KERN_INFO "内核 chrdev_write：已写入内核： %zu 字节的数据，当下偏移位位于 %lld 处。\n", cnt_write, *off);
    return cnt_write;
}

static long dev_ioctl(struct file *filp, unsigned int cmd, unsigned long arg) {
    struct cdev_private_data_t *data = filp->private_data;
    int ret = 0;
    int val = 0;
    int i   = 0;

    /* 验证命令有效性 */
    if (_IOC_TYPE(cmd) != CHRDEV_IOC_MAGIC) return -ENOTTY;
    if (_IOC_NR(cmd) > CHRDEV_IOC_MAXNR) return -ENOTTY;

    switch (cmd) {
        case CLEAR_BUF:  /* 清除缓冲区 */
            data->data_len = 0;
            memset(data->buffer, 0, data->buf_size);
            printk(KERN_INFO "ioctl: 缓冲区已清空\n");
            break;

        case GET_BUF_SIZE:  /* 获取缓冲区大小 */
            val = data->buf_size;
            if (copy_to_user((int __user *)arg, &val, sizeof(val)))
                return -EFAULT;
            break;

        case GET_DATA_LEN:  /* 获取当前数据长度 */
            val = data->data_len;
            if (copy_to_user((int __user *)arg, &val, sizeof(val)))
                return -EFAULT;
            break;

        case MAPLEAY_UPDATE_DAT_LEN:  /* 自定义：更新有效数据长度 */
            if (copy_from_user(&val, (int __user *)arg, sizeof(arg)))
                return -EFAULT;
            data->data_len = val;  //设置有效数据长度
            val = 12345678;        //特殊数字 仅用来测试 _IORW 的返回方向。
            if (copy_to_user((int __user *)arg, &val, sizeof(val)))
                return -EFAULT;
            break;
        case PRINT_BUF_DATA:
            printk(KERN_INFO"内核操作：打印当前缓冲区的值：开始：\n");
            for (i = 0; i < data->data_len; i++){
                printk(KERN_INFO "%d ", data->buffer[i]);
            }
            printk(KERN_INFO"内核操作：打印当前缓冲区的值：结束。\n");
            break;

        default:
            return -ENOTTY;
    }
    return ret;
}

static int dev_release(struct inode *inode, struct file *file) {
    printk(KERN_INFO "内核 chrdev_release：设备已被 pid 为 %d 的进程释放！\n", current->pid);
    return 0;
}

static struct file_operations fops = {
    .owner          = THIS_MODULE,
    .llseek         = dev_llseek,
    .open           = dev_open,
    .read           = dev_read,
    .write          = dev_write,
    .unlocked_ioctl = dev_ioctl,
    .release        = dev_release,
};

static int chrdev_init(void) {
    
    int err = 0;

    /* 1. 申请主设备号：动态申请方式（推荐方式） */
    if (alloc_chrdev_region(&chrdev.dev_num, MINOR_BASE, MINOR_COUNT, DEVICE_NAME))
    {
        printk(KERN_ERR"chrdev_init:alloc_chrdev_region:申请字符设备的号域失败！！！\n");
        err = -ENODEV;
        goto fail_devnum;
    }
    printk("chrdev_init: 分配主设备号： %d 次设备号： %d 成功。\n", MAJOR(chrdev.dev_num), MINOR(chrdev.dev_num));
    
    /* 2. 初始化缓冲区 */
    chrdev.dev_data.buffer = kmalloc(BUF_SIZE, GFP_KERNEL);
    if (!chrdev.dev_data.buffer) {
        printk(KERN_ERR"chrdev_init:kmalloc:申请字符设备私有数据存储空间失败！！！\n");
        err = -ENOMEM;
        goto fail_buffer;
    }
    chrdev.dev_data.buf_size = BUF_SIZE;
    /* kmalloc 成功分配内存后，返回的指针指向的内存区域包含未定义的数据（可能是旧数据或随机值）。 */
    /* kzalloc 会初始化为0，但性能弱于 kmalloc */
    memset(chrdev.dev_data.buffer, 0, chrdev.dev_data.buf_size);//可以不用执行
    chrdev.dev_data.data_len = 0;
    
    /* 3. 初始化 cdev 结构体 */
    cdev_init(&chrdev.dev, &fops);
    
    /* 4. 注册 cdev 结构体到 Linux 内核 */
    if (cdev_add(&chrdev.dev, chrdev.dev_num, MINOR_COUNT) < 0)
    {
        printk(KERN_ERR"chrdev_init:cdev_add:注册字符设备失败！！！\n");
        goto fail_cdev;
    }
    
    /* 5. 创建设备类 */
    chrdev.dev_class = class_create(THIS_MODULE, CLASS_NAME);
    if (IS_ERR(chrdev.dev_class))
    {
        err = PTR_ERR(chrdev.dev_class);
        printk(KERN_ERR"创建设备类失败！错误代码：%d\n", err);
        goto fail_class;
    }
    
    /* 6. 创建设备节点 */
    chrdev.dev_device = device_create(chrdev.dev_class, NULL, chrdev.dev_num, NULL, DEVICE_NAME);
    if (IS_ERR(chrdev.dev_device))
    {
        err = PTR_ERR(chrdev.dev_device);
        printk(KERN_ERR"创建设备节点失败！错误代码：%d\n", err);
        goto fail_device;
    }
    
    printk(KERN_INFO"字符设备驱动：已加载！\r\n"); 
    return 0;

fail_device:
    class_destroy(chrdev.dev_class);
fail_class:
    cdev_del(&chrdev.dev);
fail_cdev:
    kfree(chrdev.dev_data.buffer);
fail_buffer:
    unregister_chrdev_region(chrdev.dev_num, MINOR_COUNT);
fail_devnum:
    ;

    return err;
}

static void chrdev_exit(void) {
    
    /* 1. 销毁设备节点和设备类 */
    device_destroy(chrdev.dev_class, chrdev.dev_num);
    class_destroy(chrdev.dev_class);
    
    /* 2. 注销cdev */
    cdev_del(&chrdev.dev);
    
    /* 3. 释放设备号 */
    unregister_chrdev_region(chrdev.dev_num, MINOR_COUNT);
    
    /* 4. 释放缓冲区 */
    kfree(chrdev.dev_data.buffer);
    
    printk(KERN_INFO"字符设备驱动:已卸载！\r\n");
}

// 顶半部中断处理
static irqreturn_t gpio_key_isr(int irq, void *dev_id)
{
    struct gpio_pin_t *key = dev_id;

    // 打印中断触发信息
    printk(KERN_DEBUG "中断触发! IRQ=%d, GPIO=%d, 时间戳=%llu\n", irq, key->gpio, ktime_get_ns());
        
    // 立即禁用中断防止抖动
    disable_irq_nosync(irq);
    
    // 调度底半部处理（延迟50ms去抖）
    mod_timer(&key->debounce_timer, jiffies + msecs_to_jiffies(50));
    
    return IRQ_HANDLED;
}

// 祛除抖动算法的定时器回调函数
static void debounce_timer_handler(struct timer_list *t)
{
    struct gpio_pin_t *key = from_timer(key, t, debounce_timer);
    int val = gpiod_get_value_cansleep(key->gpiod);

    // 打印防抖检测结果
    printk(KERN_DEBUG "防抖检测: GPIO=%d, 电平=%d, 状态稳定=%s\n",
          key->gpio,
          val,
          (val == gpiod_get_value_cansleep(key->gpiod)) ? "是" : "否");

    if (val == gpiod_get_value_cansleep(key->gpiod)) {
        // 打印事件上报详情
        printk(KERN_DEBUG "上报事件: 设备=%s, 键值=KEY_POWER, 状态=%s\n",
              key->input_dev->name,
              val ? "按下" : "释放");
              
        input_report_key(key->input_dev, KEY_POWER, val);
        input_sync(key->input_dev);
    }
    
    enable_irq(key->irq);
    printk(KERN_DEBUG "重新启用中断: IRQ=%d\n", key->irq);
}

/* 
** 1. gpio 引脚初始化不用去写驱动代码，直接适配设备树硬件节点就OK了。
**    原因是，ST公司的BSP驱动工程师已经实现了 pinctrl 和 GPIO 子系统的注册。
** 2. 关于linux驱动，同样ST的BSP已经实现了很多硬件驱动的注入，包括中断子系统。
**    我们需要做的是，在设备树中适配硬件信息，且在驱动中利用中断APIs申请相应资源。
**    我用的是带 devm_xxx 前缀的中断类型 APIs，这类会多带一个 device 参数，特点是
**    内核可以主动绑定到此设备对象的生命管理周期，死后，内核及时释放相关中断资源。
** 3. 动态结构体数组的用法: 先声明结构体类型的指针，再将申请到的若干个个数的结构体
**    数组的首地址赋值给先前准备好的指针。最后，使用时，可用 pointer[i] 的形式。
** 4. 内核崩溃，引用了空指针，未分配空间的无效地址：
**        4.1 可能是设备树匹配异常导致内核未能正确自动初始化相应的对象；
**        4.2 大意的时候：没有申请有效空间，就去滥用野指针；
*/
static int pltfrm_driver_probe(struct platform_device *pdev)
{
    int count = 0; int ret; int i = 0; 
    struct device_node *node = pdev->dev.of_node; /* 便于阅读分析代码 */
    struct device *dev = &pdev->dev;              /* 便于阅读分析代码 */
    enum of_gpio_flags flag;
    char *input_name = NULL;

    /* 第一部分：按键中断的实现 */
    /* 1. 从设备树上获取特定compatible-ID的"mkey-gpios"属性硬件gpio数量 
    **    of_gpio_named_count 函数是精确查找，必须使用属性键全名，"mkey-gpios"
    **    不能使用 "mkey" 否则查不到！
    */
    count = of_gpio_named_count(node, "mkey-gpios");
    if (count < 0) {
        dev_err(dev, "设备树中缺失有效 mkey-gpio 信息！");
        return -EINVAL;
    }
    printk("成功解析出: %d 个按键gpio信息\n", count);

    /* 2. 申请自定义驱动对象(struct gpio_key类型)所需的存储空间 */
    chrdev.gpio_keys = devm_kzalloc(dev, sizeof(struct gpio_pin_t)*count, GFP_KERNEL);
    if (!chrdev.gpio_keys) {
        dev_err(dev, "无法分配 GPIO 按键内存，请求大小: %zu bytes\n", 
                sizeof(struct gpio_pin_t) * count);
        return -ENOMEM;
    }
    
    /*
    ** 分两部分：3.是纯粹为按键设置中断；A.是将按键注入到输入子系统内；
    ** 3. 解析此设备树节点内的硬件信息，并初始化，最后绑定驱动。
    **    硬件keys: 两个按键，对应引脚是 gpio G3 与 H7 俩引脚。
    ** A. 为俩按键新增输入子系统特性，配合timer防抖。
    */
    for(i =0; i < count; i++){ 
        /* 3.1 依据node节点与键值""mkey-gpios""解析出GPIO的引脚的标志（flag）和引脚编号：
        **     注意：此函数有类似版本：of_get_gpio_flags,它不需要 “mkey-gpios” 键字符串。
        **     但是，当设备树节点内含多个其他类别的键值对，比如下面的 led-gpio ，就解析失败！
        **     原因暂时无法确定，但是，我修改成of_get_named_gpio_flags版本，使用全名键
        **     字符串 "mkey-gpios" ，就可以正常识别。
        */
        chrdev.gpio_keys[i].gpio  = of_get_named_gpio_flags(node, "mkey-gpios", i, &flag);
        if (chrdev.gpio_keys[i].gpio < 0) {
            dev_err(dev, "无法获取 GPIO %d, 错误码: %d\n", i, chrdev.gpio_keys[i].gpio);
            return -EINVAL;
        }

        /* 3.2 devm_gpiod_get_index 获取 GPIO 的描述符，也可称为句柄。
        **    函数的第二个参数：
        **    默认设备树节点内键名是： gpio_suffixes[] = { "gpios", "gpio" }; 
        **    需要显示传入 NULL，不能显示传入默认值！否则找不到硬件信息！崩溃！
        ** 注意：显示输入键字符串时，应该带 "mkey" 前缀，且内核会添加 "-gpio" 后缀，
        **    所以：若第二个参数是 "mkey-gpios" 那么就错！只能是"mkey"。
        */
        chrdev.gpio_keys[i].gpiod = devm_gpiod_get_index(dev, "mkey", i, GPIOD_IN); 
        if (IS_ERR(chrdev.gpio_keys[i].gpiod)) {
            return PTR_ERR(chrdev.gpio_keys[i].gpiod);
        }
        
        chrdev.gpio_keys[i].flag  = flag & OF_GPIO_ACTIVE_LOW;

        /* 3.3 根据GPIO编号，转换出对应的中断号 */
        chrdev.gpio_keys[i].irq = gpiod_to_irq(chrdev.gpio_keys[i].gpiod);   
        if (chrdev.gpio_keys[i].irq < 0) {
            return chrdev.gpio_keys[i].irq;
        }
        
        /* 3.4 注册GPIO引脚的中断资源 */
        ret = devm_request_irq(dev, 
                        chrdev.gpio_keys[i].irq, 
                        gpio_key_isr, 
                        IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING, 
                        "mapleay_gpio_key", 
                        &chrdev.gpio_keys[i]);
        if (ret) {
            printk(KERN_ERR"注册中断失败！");
        }

        /* A. 为俩keys按键，新增输入子系统特性。附加timer防抖。*/
        // A.1 初始化定时器
        timer_setup(&chrdev.gpio_keys[i].debounce_timer, debounce_timer_handler, 0);
        
        /* 将两个按键 keys 注入到linux input子系统内 */
        // A.2 allocate memory for new input device
        chrdev.gpio_keys[i].input_dev = input_allocate_device(); 
        if (!chrdev.gpio_keys[i].input_dev) {
            dev_err(dev, "无法分配输入设备%d\n", i);
            return -ENOMEM;
        }

        // A.3 分配设备名称（必须唯一）
        input_name = devm_kasprintf(dev, GFP_KERNEL, "Mapleay-Key-%d", i);
        if (!input_name) {
            dev_err(dev, "无法分配输入设备%d名称\n", i);
            input_free_device(chrdev.gpio_keys[i].input_dev);
            return -ENOMEM;
        }
        chrdev.gpio_keys[i].input_dev->name = input_name;

        // A.4 注册前设置能力 mark device as capable of a certain event
        input_set_capability(chrdev.gpio_keys[i].input_dev, EV_KEY, KEY_POWER); 

        // A.5 注册设备 register device into input core
        if (input_register_device(chrdev.gpio_keys[i].input_dev)) { 
            input_free_device(chrdev.gpio_keys[i].input_dev);
            chrdev.gpio_keys[i].input_dev = NULL; // 标记已清理
            dev_err(dev, "注册输入设备%d失败\n", i);
            
            while (--i >= 0) { // 回滚之前已注册的设备
                input_unregister_device(chrdev.gpio_keys[i].input_dev);
            }
            return -ENODEV;
        } else {
            printk(KERN_DEBUG "成功注册输入设备: %s (GPIO%d)\n",
            chrdev.gpio_keys[i].input_dev->name,
            chrdev.gpio_keys[i].gpio);
        }
    }


    /* 第二部分：同时实现字符设备驱动控制LED小灯亮灭 */
    /* 4. 注册字符设备（非必须） ：考虑如何继续注册字符驱动。
    **    字符设备跟上面的按键中断是绝对独立的俩部分，除非有意关联。
    **    如何刻意关联：
    **    同一个节点下，新增一个LED键值对的硬件信息？嗯。好像可以。
    **    验证过，可以的。这里只使用了一个LED灯，只有一个pin引脚。
    */
    chrdev.gpio_leds = devm_kzalloc(dev, sizeof(struct gpio_pin_t), GFP_KERNEL);
    if (!chrdev.gpio_leds) {
        dev_err(dev, "无法为LED GPIO分配内存\n");
        return -ENOMEM;
    }
    
    /* 获取 LED GPIO */
    chrdev.gpio_leds->gpiod = devm_gpiod_get(dev, "mled", GPIOD_OUT_LOW);
    if (IS_ERR(chrdev.gpio_leds->gpiod)) {
        dev_err(dev, "获取硬件LED引脚信息失败！\n");
        return PTR_ERR(chrdev.gpio_leds->gpiod);
    }
    
    /* 注册字符驱动 */
    if(chrdev_init()){
        return -ENODEV;
    }

    return 0;
}

static int pltfrm_driver_remove(struct platform_device *pdev)
{
    int i = 0;
    int count = 0;
    struct device_node *node = pdev->dev.of_node;

    /* 清理输入设备与定时器 */
    count = of_gpio_named_count(node, "mkey-gpios");
    for (i = 0; i < count; i++) { // 需要获取实际的按键数量
        if (chrdev.gpio_keys[i].input_dev) {
            input_unregister_device(chrdev.gpio_keys[i].input_dev);
        }
        del_timer_sync(&chrdev.gpio_keys[i].debounce_timer);
    }
    
    /* 注销字符设备资源 */
    chrdev_exit();
    
    printk(KERN_INFO "平台设备模型: remove: 已注销！\n");
    return 0;
}


static const struct of_device_id dts_driver_of_match[] = {
    { .compatible = "Mapleay,keystest" }, /* 匹配设备树中的 compatible 值 */
    { } /* 终止符 */
};

static struct platform_driver chrdev_platform_drv = {
    .probe  = pltfrm_driver_probe,
    .remove = pltfrm_driver_remove,
    .driver = {
              /* .name 字段：只需要给不匹配的任意字符串即可触发设备树匹配。但是！！
               *             第一：不能不初始化这个字段！！
               *             第二：不能初始化为 NULL ！！ 否则Linux内核崩溃！！
               */
              .name = "not_matched_strs",
              .of_match_table = dts_driver_of_match, /* 使用设备树方式 */
    },
};

static int __init chrdev_drv_init(void)
{
    return platform_driver_register(&chrdev_platform_drv);
}

static void __exit chrdev_drv_exit(void)
{
    platform_driver_unregister(&chrdev_platform_drv);
}

module_init(chrdev_drv_init);
module_exit(chrdev_drv_exit);
MODULE_LICENSE("GPL");