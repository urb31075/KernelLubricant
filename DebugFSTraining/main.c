static struct dentry *dir = 0;
static u32 hello = 0;
static u32 sum = 0;

#define len 32 
char ker_buf[len]; 
int filevalue; 

static int add_write_op(void *data, u64 value)
{
    sum += value;
    return 0;
}
DEFINE_SIMPLE_ATTRIBUTE(add_fops, NULL, add_write_op, "%llu\n");

/* read file operation */
static ssize_t myreader(struct file *fp, char __user *user_buffer, size_t count, loff_t *position) 
{ 
     return simple_read_from_buffer(user_buffer, count, position, ker_buf, len);
} 
 
/* write file operation */
static ssize_t mywriter(struct file *fp, const char __user *user_buffer, size_t count, loff_t *position) 
{ 
        if(count > len ) 
        {
            return -EINVAL; 
        }
  
        return simple_write_to_buffer(ker_buf, len, position, user_buffer, count); 
} 
 
static const struct file_operations fops_debug = { 
        .read = myreader, 
        .write = mywriter, 
};

static int __init lkm_example_init(void) 
{
    struct dentry *junk;
    
    char *kbuffer = kmalloc(len, GFP_KERNEL);  //Выделяем в ядре
    
    
    printk(KERN_INFO "URB Kernel Probe Module INIT\n");
    dir = debugfs_create_dir("example", 0);
    if (!dir) 
    {
        printk(KERN_INFO "debugfs_example: failed to create /sys/kernel/debug/example2\n");
        return -1;
    } 
    
    junk = debugfs_create_u32("hello", 0666, dir, &hello);
    if (!junk) 
    {
        printk(KERN_INFO "debugfs_example: failed to create /sys/kernel/debug/example1/hello\n");
        return -1;
    }    
    
    junk = debugfs_create_u32("sum", 0444, dir, &sum);
    if (!junk) 
    {
        printk(KERN_ALERT "debugfs_example: failed to create /sys/kernel/debug/example/add\n");
        return -1;
    }    
 
    junk = debugfs_create_file("add",0222, dir, NULL, &add_fops);
    if (!junk) 
    {
        printk(KERN_ALERT "debugfs_example: failed to create /sys/kernel/debug/example/add\n");
        return -1;
    }    
    hello = 123;
    
    junk = debugfs_create_file("text", 0644, dir, &filevalue, &fops_debug);
    
    kfree(kbuffer);
    return 0;
}

static void __exit lkm_example_exit(void) 
{
    debugfs_remove_recursive(dir);
    printk(KERN_INFO "URB Kernel Probe Module EXIT\n");
}

