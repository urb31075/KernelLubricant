#ifndef __NVRDIMM_ALLOC_H__
#define __NVRDIMM_ALLOC_H__

#include "nvdimm_export.h"

extern int nvdimm_dev_probe(struct platform_device *);
extern int nvdimm_dev_remove(struct platform_device *);
extern void nvdimm_dev_release(struct device *);

struct platform_device nvdimm_dev = {
        .name = KBUILD_MODNAME,
        .id = PLATFORM_DEVID_NONE,
        .dev = {
                .release = nvdimm_dev_release,
        },
        .num_resources = 1,
        .resource = (struct resource []) {
                {
                        .name = "baum_nvdimm",
                        .start = 0x1080000000,
                        .end = 0x127fffffff,
                        .flags = IORESOURCE_MEM
                }
        }
};

struct platform_driver nvdimm_drv = {
        .probe = nvdimm_dev_probe,
        .remove = nvdimm_dev_remove,
        .driver = {
                .name = KBUILD_MODNAME,
        }
};

struct priv {
        struct resource *res;
        struct platform_device *pdev;
        u8 repaired;
} _priv = { 0 }, *priv = &_priv;

#if 0
#define CHUNK_MAGIC 0xdeadbeef

typedef struct chunk {
        u32 magic;
        dma_addr_t paddr;
        size_t size;
        struct list_head list;
} chunk_t;
#endif

struct chunk_list {
        struct mutex mtx;
        struct list_head list;
} chunk_list = { 0 };

struct dma_coherent_mem {
    void        *virt_base;
    dma_addr_t  device_base;
    unsigned long   pfn_base;
    int     size;
    int     flags;
    unsigned long   *bitmap;
    spinlock_t  spinlock;
    bool        use_dev_dma_pfn_offset;
};

static int dma_init_coherent_memory(phys_addr_t phys_addr, dma_addr_t device_addr,
        size_t size, int flags,
        struct dma_coherent_mem **mem)
{
    struct dma_coherent_mem *dma_mem = NULL;
    void *mem_base = NULL;
    int pages = size >> PAGE_SHIFT;
    int bitmap_size = BITS_TO_LONGS(pages) * sizeof(long);

    if ((flags & (DMA_MEMORY_MAP | DMA_MEMORY_IO)) == 0)
        goto out;
    if (!size)
        goto out;

    mem_base = memremap(phys_addr, size, ARCH_MEMREMAP_PMEM);
    if (!mem_base)
        goto out;

    dma_mem = kzalloc(sizeof(struct dma_coherent_mem), GFP_KERNEL);
    if (!dma_mem)
        goto out;
    dma_mem->bitmap = kzalloc(bitmap_size, GFP_KERNEL);
    if (!dma_mem->bitmap)
        goto out;

    dma_mem->virt_base = mem_base;
    dma_mem->device_base = device_addr;
    dma_mem->pfn_base = PFN_DOWN(phys_addr);
    dma_mem->size = pages;
    dma_mem->flags = flags;
    spin_lock_init(&dma_mem->spinlock);

    *mem = dma_mem;

    if (flags & DMA_MEMORY_MAP)
        return DMA_MEMORY_MAP;

    return DMA_MEMORY_IO;

out:
    kfree(dma_mem);
    if (mem_base)
        memunmap(mem_base);
    return 0;
}

static int dma_assign_coherent_memory(struct device *dev,
        struct dma_coherent_mem *mem)
{
    if (dev->dma_mem)
        return -EBUSY;

    dev->dma_mem = mem;
    /* FIXME: this routine just ignores DMA_MEMORY_INCLUDES_CHILDREN */

    return 0;
}

static void dma_release_coherent_memory(struct dma_coherent_mem *mem)
{
    if (!mem)
        return;
    memunmap(mem->virt_base);
    kfree(mem->bitmap);
    kfree(mem);
}

static int baum_dma_declare_coherent_memory(struct device *dev, phys_addr_t phys_addr,
        dma_addr_t device_addr, size_t size, int flags)
{
    struct dma_coherent_mem *mem;
    int ret;

    ret = dma_init_coherent_memory(phys_addr, device_addr, size, flags, &mem);
    if (ret == 0)
        return 0;

    if (dma_assign_coherent_memory(dev, mem) == 0)
        return ret;

    dma_release_coherent_memory(mem);

    return 0;
}

#endif // __NVRDIMM_ALLOC_H__
