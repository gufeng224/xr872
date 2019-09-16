#ifndef _HARDWARE_PLATFORM_H_
#define _HARDWARE_PLATFORM_H_

//#include <hardware/pal_gpio.h>
//#include <hardware/pal_i2c.h>
//#include <hardware/pal_spi.h>
//#include <hardware/pal_uart.h>
//#include <hardware/pal_usb.h>
#include "pal_asound.h"
//#include <hardware/pal_irq.h>

#define CHK_DEV_STATUS  0x1

#define STATUS_OK 0x0
#define STATUS_EPERM 0x1
#define STATUS_ENOENT 0x2
#define STATUS_EIO 0x5
#define STATUS_EBADF 0x9
#define STATUS_EAGAIN 0xb
#define STATUS_EACCES 0xd
#define STATUS_EBUSY 0x10
#define STATUS_ENODEV 0x13
#define STATUS_EINVAL 0X16

#define DEV_RDONLY  0x0001
#define DEV_WRONLY  0x0002
#define DEV_RDWR    0x0004

#define MAX_OPEN_DEV_NUM 128

/*union device_resource {
    struct gpio_resource gpio_res;
    struct i2c_resource i2c_res;
    struct spi_resource spi_res;
    struct uart_resource uart_res;
    struct usb_resource usb_res;
    struct asound_resource asound_res;
};

struct resource {
    const char *name;
    unsigned flag;
    union device_resource dev_res;
    struct resource *prev;
    struct resource *next;
};

struct platform_device {
    const char *name;
    int	num_resources;
    struct resource *resource;
};

struct dev_fops {
    int (*read)(char *buf, unsigned size);
    int (*write)(char *buf, unsigned size);
    int (*ioctl)(unsigned cmd, void *pvalue);
};

struct dev_pm_ops {
    int (*suspend)(struct platform_device *pdev);
    int (*resume)(struct platform_device *pdev);
    int (*runtime_suspend)(struct platform_device *pdev);
    int (*runtime_resume)(struct platform_device *pdev);
};

struct platform_driver {
    const char *name;
    int (*probe)(struct platform_device *pdev);
    int (*remove)(struct platform_device *pdev);
    struct dev_pm_ops pm_ops;
    struct dev_fops fops;
};

struct platform_device_driver {
    struct platform_device *pdev;
    struct platform_driver *pdrv;
    struct platform_device_driver *prev;
    struct platform_device_driver *next;
    unsigned id;
    int open_cnt;
};
*/

extern int platform_device_driver_bind(struct platform_device *pdevice, struct platform_driver *pdriver);
/*
extern int platform_device_chain_remove(void);
extern int platform_device_chain_suspend(void);
extern int platform_device_chain_resume(void);
*/
extern int platform_device_open(const char *dev_name, unsigned flag);
extern int platform_device_read(int fd, char *buf, unsigned size);
extern int platform_device_write(int fd, char *buf, unsigned size);
extern int platform_device_ioctl(int fd, unsigned cmd, void *pvalue);
extern int platform_device_close(int fd);

#endif /*_HARDWARE_PLATFORM_H_*/
