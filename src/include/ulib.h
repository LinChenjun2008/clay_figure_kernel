#ifndef __ULIB_H__
#define __ULIB_H__

PUBLIC void exit(int status);
PUBLIC int  get_pid(void);
PUBLIC int  get_ppid(void);
PUBLIC int  create_process(const char *name, void *proc);

PUBLIC void *allocate_page(void);
PUBLIC void  free_page(void *addr);
PUBLIC void  read_task_addr(pid_t pid, void *addr, size_t size, void *buffer);

PUBLIC uint64_t get_ticks(void);

PUBLIC void fill(
    void    *buffer,
    size_t   buffer_size,
    uint32_t xsize,
    uint32_t ysize,
    uint32_t x,
    uint32_t y
);

#endif