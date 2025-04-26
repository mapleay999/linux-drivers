
KDIR := /home/ericedward/linux_space/linux_kernel/my_linux_kernel/linux-stm32mp-5.4.31-r0/linux-5.4.31/
CURRENT_PATH := $(shell pwd)
TARGET_PATH  := /home/ericedward/linux_space/tools/nfs/rootfs/home/maple/linux_driver/chrdev/01_chrdev/
WORKING_PATH := /home/ericedward/linux_space/linux_driver/chrdev/01_chrdev

obj-m += chrdev_platfrom_driver.o
ccflags-y += -I$(WORKING_PATH)/include

all:
	$(MAKE) -C $(KDIR) M=$(CURRENT_PATH) modules

clean:
	$(MAKE) -C $(KDIR) M=$(CURRENT_PATH) clean # 正在删除 Linux 字符设备驱动的编译产出文件……
	rm -f **.out **.a  # 正在删除应用程序的编译产出文件……

testapp:testapp.c
	arm-none-linux-gnueabihf-gcc -I$(WORKING_PATH)/include $(WORKING_PATH)/testapp.c -o $(WORKING_PATH)/testapp.out

deploy:
	sudo cp $(CURRENT_PATH)/**.ko  $(TARGET_PATH)
	sudo cp $(CURRENT_PATH)/**.out $(TARGET_PATH)