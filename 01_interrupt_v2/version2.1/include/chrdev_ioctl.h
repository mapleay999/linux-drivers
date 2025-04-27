#ifndef __CHRDEV_IOCTL_H__
#define __CHRDEV_IOCTL_H__

#define CHRDEV_IOC_MAGIC   'k'
#define CLEAR_BUF              _IO(CHRDEV_IOC_MAGIC, 0)
#define GET_BUF_SIZE           _IOR(CHRDEV_IOC_MAGIC, 1, int)
#define GET_DATA_LEN           _IOR(CHRDEV_IOC_MAGIC, 2, int)
#define MAPLEAY_UPDATE_DAT_LEN _IOWR(CHRDEV_IOC_MAGIC, 3, int)
#define PRINT_BUF_DATA         _IO(CHRDEV_IOC_MAGIC, 4)
#define CHRDEV_IOC_MAXNR    4

#endif