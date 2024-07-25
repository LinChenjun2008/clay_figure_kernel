#ifndef __BITMAP_H__
#define __BITMAP_H__

typedef struct
{
    int32_t  btmp_bytes_len;
    uint8_t *map;
} bitmap_t;

PUBLIC void init_bitmap(bitmap_t *btmp);
PUBLIC bool bitmap_scan_test(bitmap_t *btmp,int32_t bit_index);
PUBLIC status_t bitmap_alloc(IN(bitmap_t *btmp,int32_t cnt),OUT(uint32_t *index));
PUBLIC void bitmap_set(bitmap_t *btmp,int32_t bit_index,uint8_t value);

#endif