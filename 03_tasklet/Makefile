
KDIR := /home/ericedward/linux_space/linux_kernel/my_linux_kernel/linux-stm32mp-5.4.31-r0/linux-5.4.31/
TARGET_PATH  := /home/ericedward/linux_space/tools/nfs/rootfs/home/maple/linux_driver/chrdev/01_chrdev/

obj-m += tasklet_demo.o

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean # 正在删除 Linux 字符设备驱动的编译产出文件……
	rm -f **.out **.a  # 正在删除应用程序的编译产出文件……

deploy:
	sudo cp $(PWD)/**.ko  $(TARGET_PATH)
