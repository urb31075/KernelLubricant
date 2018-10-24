#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/debugfs.h>
#include <linux/slab.h>
#include "nvdimm_export.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("URB31075");
MODULE_DESCRIPTION("Unit tesk kernel level Linux module.");
MODULE_VERSION("0.01");

extern void urb_test_01(void);
extern void urb_test_02(void);

static int __init execute_unit_tests(void) 
{
    printk(KERN_INFO "URB Kernel Level Unit Test START\n");
    //urb_test_01(); 
    //urb_test_02(); 
    printk(KERN_INFO "Testing nvdimm_alloc:\n");
    //==============================================================================================================
    void* x = nvdimm_alloc(100, 0);
    if (!x)
    {
         printk(KERN_INFO "nvdimm_alloc return NULL\n");         
    }
    
    printk(KERN_INFO "URB Kernel Level Unit Test FINISH\n"); 
    return -1;
}

module_init(execute_unit_tests); //Запускаем UNIT-tests заглушки драйвера NVDIMM



