#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>
#include <linux/dmapool.h>
#include <linux/device.h>
#include <linux/string.h>
#include <linux/list.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/types.h>
//#include <linux/pmem.h>
#include "nvdimm_alloc.h"

#define DRV KBUILD_MODNAME ": "

#ifdef DEBUG
#define dbg(fmt, ...) printk(DRV fmt, ##__VA_ARGS__)
#else
#define dbg(fmt, ...) do { } while(0)
#endif

static unsigned long nvdimm_usage = 0;

#if 0
void nvdimm_recover(void);
struct list_head *nvdimm_get_list_head(void);
#endif

/* --- chunk API start --- */
void *chunk_get_data_addr(chunk_t *chunk)
{
    return (void *) chunk + sizeof(chunk_t);
}
EXPORT_SYMBOL(chunk_get_data_addr);

static size_t chunk_real_size(chunk_t *chunk)
{
    return chunk->size + sizeof(chunk_t);
}

chunk_t *chunk_find(void *addr)
{
        chunk_t *chunk = NULL;

        mutex_lock(&chunk_list.mtx);
        list_for_each_entry(chunk, &chunk_list.list, list) {
                if ((void *) chunk == (addr - sizeof(chunk_t))) {
                        mutex_unlock(&chunk_list.mtx);
                        return chunk;
                }
        }
        mutex_unlock(&chunk_list.mtx);

        return NULL;
}
EXPORT_SYMBOL(chunk_find);

static void chunk_add(chunk_t *chunk)
{
        mutex_lock(&chunk_list.mtx);
        list_add_tail(&chunk->list, &chunk_list.list);
        if (!chunk->recovered)
            nvdimm_usage += PAGE_ALIGN(chunk_real_size(chunk));
        mutex_unlock(&chunk_list.mtx);
}

void chunk_delete(chunk_t *chunk)
{
    if (chunk->recovered) {
        size_t len = chunk_real_size(chunk);
        dbg("free recovered chunk [%zu] bytes\n", PAGE_ALIGN(chunk->size));
        list_del(&chunk->list);
        memset(chunk, 0, len);
        kfree((void *) chunk);
    } else {
        dbg("free [%zu] bytes at [%llx] address\n", PAGE_ALIGN(chunk->size), chunk->paddr);
        nvdimm_usage -= PAGE_ALIGN(chunk_real_size(chunk));
        list_del(&chunk->list);
   }
}
EXPORT_SYMBOL(chunk_delete);

void chunk_delete_locked(chunk_t *chunk)
{
        mutex_lock(&chunk_list.mtx);
        chunk_delete(chunk);
        mutex_unlock(&chunk_list.mtx);
}
EXPORT_SYMBOL(chunk_delete_locked);

/*static void chunk_delete_all(void)
{
        chunk_t *n, *tmp;

        mutex_lock(&chunk_list.mtx);
        list_for_each_entry_safe(n, tmp, &chunk_list.list, list) {
            nvdimm_free_by_chunk_addr(n, 0);
        }
        mutex_unlock(&chunk_list.mtx);
}*/
/* --- chunk API end --- */

int nvdimm_dev_probe(struct platform_device *pdev)
{
        int ret = 0;
        
        struct resource *mem;
        struct device *dev = &pdev->dev;

        mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
        if (!mem) {
                dev_err(dev, "platform_get_resource err\n");
                return -EINVAL;
        }

        priv->pdev = pdev;
        priv->res = mem;
        priv->repaired = 0;

        //platform_set_drvdata(pdev, priv);

        ret = baum_dma_declare_coherent_memory(dev, mem->start, mem->start, resource_size(mem), DMA_MEMORY_MAP | DMA_MEMORY_EXCLUSIVE); //debug_urb
        if (ret != DMA_MEMORY_MAP) 
        {
                dev_err(dev, "unable to declare DMA memory %d\n", ret);
                goto err_devm_kzalloc;
        }

        if (dma_coerce_mask_and_coherent(dev, DMA_BIT_MASK(64))) {
                ret = -ENODEV;
                dev_err(dev, "dma_coerce_mask_and_coherent err\n");
                goto err_dma;
        }
        if (dma_set_mask_and_coherent(dev, DMA_BIT_MASK(64))) {
                ret = -ENODEV;
                dev_err(dev, "dma_set_mask_and_coherent err\n");
                goto err_dma;
        }

        mutex_init(&chunk_list.mtx);
        INIT_LIST_HEAD(&chunk_list.list);

#if 1
        nvdimm_recover();
#if 0
        chunk_t *chunk, *tmp;
        struct list_head *head = nvdimm_get_list_head();

        list_for_each_entry_safe(chunk, tmp, head, list) {
            u32 l = readl(chunk_get_data_addr(chunk));
            printk("data %x\n", l);
            chunk_utilize(chunk);
        }
#endif
#endif

        return 0;

err_dma:
        dma_release_declared_memory(dev);

err_devm_kzalloc:
        return ret;
}

int nvdimm_dev_remove(struct platform_device *pdev)
{
    //chunk_delete_all();
    mutex_destroy(&chunk_list.mtx);

    dma_release_declared_memory(&pdev->dev);

    return 0;
}

void nvdimm_dev_release(struct device *dev)
{
}

u8 nvdimm_recovered(void)
{
    return priv->repaired;
}
EXPORT_SYMBOL(nvdimm_recovered);

void nvdimm_recover(void)
{
    struct device *dev = &priv->pdev->dev;
    size_t res_len = resource_size(priv->res);
    void *base = dev->dma_mem->virt_base;
    phys_addr_t offset;

    for (offset = 0; offset < res_len; offset += PAGE_SIZE) {
        if (readl(base + offset) == CHUNK_MAGIC) {
            chunk_t *chunk = base + offset;
            chunk_t *new_chunk;

            new_chunk = kzalloc(chunk_real_size(chunk), GFP_KERNEL);
            if (!new_chunk) {
                dbg("could not allocate memory for chunk\n");
                continue;
            }

            memcpy(new_chunk, chunk, chunk_real_size(chunk));
            new_chunk->recovered = 1;
            chunk_add(new_chunk);
            // !!! fix it at release
            memset(chunk, 0, chunk_real_size(chunk));

            dbg("recovered data at 0x%llx, size %zu\n", new_chunk->paddr, new_chunk->size);
        }
    }

    priv->repaired = 1;
}
EXPORT_SYMBOL(nvdimm_recover);

void *nvdimm_alloc(size_t size, int flags)
{
        void *addr;
        dma_addr_t dma_addr;
        struct device *dev = &priv->pdev->dev;
        chunk_t *chunk = NULL;

        addr = dma_alloc_coherent(dev, size + sizeof(chunk_t), &dma_addr, flags);
        if (addr) {
                dbg("allocate [%zu] bytes at address [%llx] %llx\n", PAGE_ALIGN(size), dma_addr, addr);
                memset(addr, 0, size + sizeof(chunk_t));
        } else {
                dbg("could not allocate buffer\n");
                return NULL;
        }

        chunk = addr;

        chunk->magic = CHUNK_MAGIC;
        chunk->paddr = dma_addr;
        chunk->size = size;
        chunk->recovered = 0;

        chunk_add(chunk);

        return addr + sizeof(chunk_t);
}
EXPORT_SYMBOL(nvdimm_alloc);

void nvdimm_free_by_chunk_addr(chunk_t *chunk, bool locked)
{
        struct device *dev = &priv->pdev->dev;
        size_t len;
        dma_addr_t paddr;

        if (!chunk)
                return;

        len = chunk_real_size(chunk);
        paddr = chunk->paddr;

        if (locked)
            chunk_delete_locked(chunk);
        else
            chunk_delete(chunk);

        memset(chunk, 0, len);
        dma_free_coherent(dev, len, (void *) chunk, paddr);
}
EXPORT_SYMBOL(nvdimm_free_by_chunk_addr);

void nvdimm_free_by_data_addr(void *addr, bool locked)
{
        chunk_t *chunk = NULL;

        chunk = chunk_find(addr);
        if (!chunk) {
                dbg("!! could not find appropriate address [%llx] in chunk_list\n", addr);
                return;
        }

        nvdimm_free_by_chunk_addr(chunk, locked);
}
EXPORT_SYMBOL(nvdimm_free_by_data_addr);

// get percentage of available space
unsigned short nvdimm_percentage(void)
{
        return (100 * (resource_size(priv->res) - nvdimm_usage)) / resource_size(priv->res);
}
EXPORT_SYMBOL(nvdimm_percentage);

struct list_head *nvdimm_get_list_head(void)
{
        return &chunk_list.list;
}
EXPORT_SYMBOL(nvdimm_get_list_head);

void nvdimm_list_mutex_lock(void)
{
        mutex_lock(&chunk_list.mtx);
}
EXPORT_SYMBOL(nvdimm_list_mutex_lock);

void nvdimm_list_mutex_unlock(void)
{
        mutex_unlock(&chunk_list.mtx);
}
EXPORT_SYMBOL(nvdimm_list_mutex_unlock);

static int __init nvdimm_drv_init(void)
{
    printk(KERN_INFO "NVDIMM STUB INIT\n");
    return 0; //debug_urb    
    /*    int ret;

        ret = platform_driver_register(&nvdimm_drv);
        if (ret) {
                printk("platform_driver_register ret %d\n", ret);
                goto err_driver_register;
        }

        ret = platform_device_register(&nvdimm_dev);
        if (ret) {
                printk("platform_device_register ret %d\n", ret);
                platform_driver_unregister(&nvdimm_drv);
                goto err_driver_register;
        }

        printk(DRV "loaded\n");

        return 0;

    err_driver_register:
        return ret;*/
}

static void __exit nvdimm_drv_exit(void)
{
        platform_device_unregister(&nvdimm_dev);
        platform_driver_unregister(&nvdimm_drv);

        printk(DRV "unloaded\n");
}

module_init(nvdimm_drv_init);
module_exit(nvdimm_drv_exit);

MODULE_AUTHOR("Sergey Katyshev <s.katyshev@npobaum.ru, perplexus@ya.ru>");
MODULE_LICENSE("Dual BSD/GPL");
MODULE_DESCRIPTION("This module provides API to allocate memory at NVDIMM memory");

