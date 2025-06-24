#ifndef __ULIB_H__
#define __ULIB_H__

PUBLIC uint64_t get_ticks(void);

PUBLIC void *allocate_page(uint32_t count);
PUBLIC void  free_page(void *addr, uint32_t count);
PUBLIC void  read_prog_addr(pid_t pid, void *addr, size_t size, void *buffer);

PUBLIC void put_pixel(uint32_t x, uint32_t y, uint32_t color);
PUBLIC void fill(
    void    *buffer,
    size_t   buffer_size,
    uint32_t xsize,
    uint32_t ysize,
    uint32_t x,
    uint32_t y
);

#endif