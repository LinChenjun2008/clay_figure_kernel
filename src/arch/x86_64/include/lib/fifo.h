/*
   Copyright 2024 LinChenjun

   本程序是自由软件
   修改和/或再分发依照 GNU GPL version 3 (or any later version)

*/

#ifndef __FIFO_H__
#define __FIFO_H__

#include <device/spinlock.h>

typedef struct fifo_s
{
    void *data;
    int   item_size; /* 元素大小 */
    int   size;      /* 大小(最大元素数) */
    int   free;
    int   next_read;
    int   next_write;

    spinlock_t lock;
} fifo_t;

/**
 * @brief 初始化一个fifo
 * @param fifo 将要初始化的fifo指针
 * @param data fifo数据存储区
 * @param item_size fifo内各项的大小
 * @param size fifo可容纳的项个数
 */
PUBLIC void init_fifo(fifo_t *fifo, void *data, size_t item_size, int size);

/**
 * @brief 将item写入fifo中
 * @param fifo 将被写入的fifo指针
 * @param item 要被写入的item指针
 * @return 成功将返回K_SUCCESS,若失败则返回对应的错误码
 */
PUBLIC status_t fifo_write(fifo_t *fifo, void *item);

/**
 * @brief 从fifo中读取item
 * @param fifo 被读取的fifo指针
 * @param item 获取读出item的指针
 * @return 成功将返回K_SUCCESS,若失败则返回对应的错误码
 */
PUBLIC status_t fifo_read(fifo_t *fifo, void *item);

PUBLIC bool fifo_empty(fifo_t *fifo);
PUBLIC bool fifo_full(fifo_t *fifo);

#endif
