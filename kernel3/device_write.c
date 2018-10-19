#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include "device_write.h"

MODULE_LICENSE("GPL");
void debug_write_operation(void)
    {
        printk(KERN_INFO "URB debug_write_operation.\n");
    }

