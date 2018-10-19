#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/uaccess.h>

#include "device_write.h"
#include "device_read.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("r.ugryumov");
MODULE_DESCRIPTION("Debug stub for SWARM kernel module.");
MODULE_VERSION("0.01");
#define DEVICE_NAME "kernel_stub"

/* Prototypes for device functions */
static int device_open(struct inode *, struct file *);
static int device_release(struct inode *, struct file *);
static ssize_t device_read(struct file *, char *, size_t, loff_t *);
static ssize_t device_write(struct file *, const char *, size_t, loff_t *);
static int major_num;
static int device_open_count = 0;


/* This structure points to all of the device functions */
static struct file_operations file_ops = 
{
 .read = device_read,
 .write = device_write,
 .open = device_open,
 .release = device_release
};

/* When a process reads from our device, this gets called. */
static ssize_t device_read(struct file *flip, char *buffer, size_t len, loff_t *offset) 
{
    printk(KERN_INFO "URB device_read\n");    
    ssize_t bytesread = debug_read_operation(flip, buffer, len, offset);
    return bytesread;
}

static ssize_t device_write(struct file *flip, const char *buffer, size_t len, loff_t *offset) 
{
    printk(KERN_INFO "URB device_write\n");
    debug_write_operation();    
    return len;
}

/* Called when a process opens our device */
static int device_open(struct inode *inode, struct file *file) 
{
     /* If device is open, return busy */
    if (device_open_count) 
    {
        return -EBUSY;
    }
    
    device_open_count++;
    try_module_get(THIS_MODULE);
    return 0;
}

/* Called when a process closes our device */
static int device_release(struct inode *inode, struct file *file) 
{
    device_open_count--; /* Decrement the open counter and usage count. Without this, the module would not unload. */
    module_put(THIS_MODULE);
    return 0;
}

static int __init lkm_example_init(void) 
{
    printk(KERN_INFO "URB Kernel Module INIT");
    major_num = register_chrdev(0, "lkm_example", &file_ops); /* Try to register character device */
    if (major_num < 0) 
    {
        printk(KERN_INFO "URB Could not register device: %d\n", major_num);
        return major_num;
    } 
    else 
    { 
        printk(KERN_INFO "URB major_num %d\n", major_num);
        return 0;
    }
}

static void __exit lkm_example_exit(void) 
{
    printk(KERN_INFO "URB Kernel Module EXIT\n");
    unregister_chrdev(major_num, DEVICE_NAME); /* Remember — we have to clean up after ourselves. Unregister the character device. */
}

/* Register module functions */
module_init(lkm_example_init);
module_exit(lkm_example_exit);

