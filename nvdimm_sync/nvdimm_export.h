#ifndef __NVRDIMM_EXPORT_H__
#define __NVRDIMM_EXPORT_H__

#define CHUNK_MAGIC 0xdeadbeef

typedef struct chunk {
        u32 magic;
        u8 recovered;
        dma_addr_t paddr;
        size_t size;
        struct list_head list;
} chunk_t;

extern void *nvdimm_alloc(size_t size, int flags);
extern void nvdimm_free_by_chunk_addr(chunk_t *chunk, bool locked);
extern void nvdimm_free_by_data_addr(void *addr, bool locked);
extern unsigned short nvdimm_percentage(void);
extern struct list_head *nvdimm_get_list_head(void);
extern void *chunk_get_data_addr(chunk_t *chunk);
extern void chunk_utilize(chunk_t *chunk);
extern void chunk_delete_locked(chunk_t *chunk);
extern void chunk_delete(chunk_t *chunk);
extern u8 nvdimm_recovered(void);
extern void nvdimm_recover(void);
extern chunk_t *chunk_find(void *addr);
extern void nvdimm_list_mutex_lock(void);
extern void nvdimm_list_mutex_unlock(void);

#endif // __NVRDIMM_EXPORT_H__
