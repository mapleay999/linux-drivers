#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include "chrdev_ioctl.h"

#define DEVICE_FILE "/dev/mapleay-chrdev-device"
#define MAX_INPUT_LEN 128

void print_usage() {
    printf("\n支持的命令：\n");
    printf("  w   <string>      写入数据的值\n");
    printf("  r   <string:x>    读取x个数据\n");
    printf("  l   <string:y>    更新文件位置为y\n");
    printf("  c                 清空缓冲区\n");
    printf("  buf_size          获取缓冲区大小\n");
    printf("  data_len          获取当前数据长度\n");
    printf("  update_len <长度> 更新数据长度\n");
    printf("  p                 请内核中打印缓冲区数据\n");
    printf("  help              显示帮助信息\n");
    printf("  exit              退出程序\n");
    printf(">> ");
    fflush(stdout);
}

int main() {
    
    char input[MAX_INPUT_LEN];
    char cmd[MAX_INPUT_LEN];
    char param[MAX_INPUT_LEN];
    
    int fd = open(DEVICE_FILE, O_RDWR, 0777);
    if (fd < 0) {
        perror("应用层：打开设备文件失败！");
        return -1;
    }

    printf("测试应用程序已启动（输入 help 显示帮助）：\n");
    printf(">> ");

    while (1) {
        // 读取用户输入
        if (fgets(input, MAX_INPUT_LEN, stdin) == NULL) {
            break; // 处理 EOF (Ctrl+D)
        }

        // 去除换行符并分割命令参数
        input[strcspn(input, "\n")] = '\0';
        int num_args = sscanf(input, "%s %s", cmd, param);

        // 处理空输入
        if (strlen(input) == 0) {
            print_usage();
            continue;
        }

        // 解析命令
        if (strcmp(cmd, "exit") == 0) {
            printf("正在退出程序...\n");
            break;
        } else if (strcmp(cmd, "help") == 0) {
            print_usage();
        } else if (strcmp(cmd, "w") == 0) {                /* write */
            if (num_args < 2) {
                printf("错误：缺少写入数据，用法：w <数据>\n");
                print_usage();
                continue;
            }
            char data_in = atoi(param); /* 返回为 int 类型变量，与内核存储变量一致 */
            /* 提醒：Unix C 的 write 系统调用函数，没有原生修改文件 offset 的能力。 */
            ssize_t cnt_written = write(fd, &data_in, sizeof(data_in));
            if (cnt_written < 0) {
                perror("写入内核失败！");
            } else {
                printf("成功写入 %zd 字节\n", cnt_written);
            }
        } else if (strcmp(cmd, "r") == 0) {                /* read */
            /* 提醒：Unix C 的 read 系统调用函数，没有原生修改文件 offset 的能力。 */
            if (num_args < 2) {
                printf("错误：缺少读出数据位置，用法：w <数据>\n");
                print_usage();
                continue;
            }
            int cnt_to_read = atoi(param);
            char read_buf[1024];
            ssize_t cnt_read = read(fd, read_buf, cnt_to_read);
            if (cnt_read < 0) {
                perror("读取内核失败！");
            } else {
                printf("读取到 %zd 字节：\n", cnt_read);
                for (int i = 0; i < cnt_read; i++) {
                    printf("%d ", read_buf[i]);
                }
                printf("\n");
            }
        } else if (strcmp(cmd, "l") == 0) {                         /* lseek */
            if (num_args < 2) {
                printf("错误：缺少位置参数，用法：lseek <位置>\n");
                print_usage();
                continue;
            }
            int data_pos = atoi(param);
            /* 提示：有类型转换： loff_t 转 off_t */
            off_t new_pos = lseek(fd, data_pos, SEEK_SET);
            if (new_pos < 0) {
                perror("更新文件位置失败");
            } else {  //loff_t实为 long long 类型
                printf("当前文件位置更新为：%lld\n", (long long)new_pos);
            }
        } else if (strcmp(cmd, "c") == 0) {
            if (ioctl(fd, CLEAR_BUF) < 0) {
                perror("清空缓冲区失败");
            } else {
                printf("缓冲区已清空\n");
            }
        } else if (strcmp(cmd, "buf_size") == 0) {
            int size;
            if (ioctl(fd, GET_BUF_SIZE, &size) < 0) {
                perror("获取缓冲区大小失败");
            } else {
                printf("缓冲区总大小：%d 字节\n", size);
            }
        } else if (strcmp(cmd, "data_len") == 0) {
            int len;
            if (ioctl(fd, GET_DATA_LEN, &len) < 0) {
                perror("获取数据长度失败");
            } else {
                printf("当前数据长度：%d 字节\n", len);
            }
        } else if (strcmp(cmd, "update_len") == 0) {
            if (num_args < 2) {
                printf("错误：缺少长度参数，用法：u <长度>\n");
                print_usage();
                continue;
            }
            int len = atoi(param);
            if (ioctl(fd, MAPLEAY_UPDATE_DAT_LEN, &len) < 0) {
                perror("更新数据长度失败");
            } else {
                printf("返回内核底层的特殊状态码：%d\n", len);
            }
        } else if (strcmp(cmd, "p") == 0) {
            if (ioctl(fd, PRINT_BUF_DATA) < 0) {
                perror("请内核中打印缓冲区数据失败");
            }
        } else {
            printf("未知命令：%s\n", cmd);
            print_usage();
        }

        printf(">> ");
        fflush(stdout);
    }

    close(fd);
    sleep(1);
    return 0;
}