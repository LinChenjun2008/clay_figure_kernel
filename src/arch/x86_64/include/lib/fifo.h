/*
   Copyright 2024 LinChenjun

   本程序是自由软件
   修改和/或再分发依照 GNU GPL version 3 (or any later version)

*/

#ifndef __FIFO_H__
#define __FIFO_H__

typedef struct fifo_s
{
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
PUBLIC status_t fifo_put(fifo_t *fifo,void* data);

PUBLIC status_t fifo_get(fifo_t *fifo,void* data);
PUBLIC bool fifo_empty(fifo_t *fifo);
PUBLIC bool fifo_fill(fifo_t *fifo);

#endif
