#ifndef __FIFO_H__
#define __FIFO_H__

typedef struct
{
    // struct lock lock;
    union
    {
        uint8_t  *buf8;
        uint16_t *buf16;
        uint32_t *buf32;
        uint64_t *buf64;
    };
    int type; /* 类型(8,16,32或64) */
    int size; /* 大小(最大元素数) */
    int free;
    int nr;
    int nw;
} fifo_t;

PUBLIC void init_fifo(fifo_t *fifo,void* buf,int type,int size);
PUBLIC status_t fifo_put(IN(fifo_t *fifo,void* data));

PUBLIC status_t fifo_get(IN(fifo_t *fifo),OUT(void* data));
PUBLIC bool fifo_empty(fifo_t *fifo);
PUBLIC bool fifo_fill(fifo_t *fifo);

#endif
