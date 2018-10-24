#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include "device_write.h"
#include "debug_nvdimm.h"
extern char DebugNVDIMMBuffer[DEBUG_NVDIMM_BUFFER_LENGTH]; //Отладочный буффер памяти эмулирующий NVDIMM
extern int DebugNVDIMMBufferAmount; //Колличество байт данных в отладочном буфере

void write_nvdimm_stub(const char *buffer, size_t len)
    {
        char *out_str = kmalloc(len+1, GFP_KERNEL);
        memset(out_str, 0, len+1);
        memcpy(out_str, buffer, len);
        printk(KERN_INFO "URB debug_write_operation: %s  size: %ld\n", out_str, len);
        kfree(out_str);
        
        memset(DebugNVDIMMBuffer, 0, DEBUG_NVDIMM_BUFFER_LENGTH); //Обнуляем для удобства работы
        memcpy(DebugNVDIMMBuffer, buffer, len);
        DebugNVDIMMBufferAmount = len;
    }

MODULE_LICENSE("GPL");
