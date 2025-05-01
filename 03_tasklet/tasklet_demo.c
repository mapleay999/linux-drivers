#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/printk.h>

// 定义 tasklet 结构体
static struct tasklet_struct my_tasklet;

// Tasklet 处理函数
static void my_tasklet_handler(unsigned long data)
{
    printk(KERN_INFO "Tasklet executed on CPU %d\n", smp_processor_id());
}

// 模块初始化
static int __init tasklet_module_init(void)
{
    printk(KERN_INFO "Initializing tasklet module\n");
    
    // 初始化 tasklet（绑定处理函数）
    tasklet_init(&my_tasklet, my_tasklet_handler, 0);
    
    // 调度 tasklet（立即加入执行队列）
    tasklet_schedule(&my_tasklet);
    
    return 0;
}

// 模块退出
static void __exit tasklet_module_exit(void)
{
    // 禁用并等待 tasklet 完成
    tasklet_kill(&my_tasklet);
    printk(KERN_INFO "Tasklet module removed\n");
}

module_init(tasklet_module_init);
module_exit(tasklet_module_exit);

MODULE_LICENSE("GPL");