#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include "device_read.h"
#include "debug_nvdimm.h"

extern char DebugNVDIMMBuffer[DEBUG_NVDIMM_BUFFER_LENGTH]; //Отладочный буффер памяти эмулирующий NVDIMM
extern int DebugNVDIMMBufferAmount; //Колличество байт данных в отладочном буфере

ssize_t get_nvdimm_stub_data_amount(void)
{
    return DebugNVDIMMBufferAmount;
}

ssize_t read_nvdimm_stub(char *buffer)
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
        
        memcpy(buffer, DebugNVDIMMBuffer, DebugNVDIMMBufferAmount);
        return DebugNVDIMMBufferAmount;        
    }

MODULE_LICENSE("GPL");
