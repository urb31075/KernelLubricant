#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include "device_read.h"

MODULE_LICENSE("GPL");
ssize_t debug_read_operation(struct file *flip, char *buffer, size_t len, loff_t *offset)
    {
        static int finished = 0;

        if (finished) 
        {
            printk(KERN_INFO "URB Read End\n");
            finished = 0;
            return 0;
        }

        printk(KERN_INFO "URB debug_read_operation.\n");
   
        finished = 1;    
    
        char kbuf[]="0123456789";
        int i;
        for (i= 0; i< sizeof(kbuf); i++)
        {
            put_user(*(kbuf+i), buffer++);        
        }

        return sizeof(kbuf);        
    }
