#ifndef __ALLOC_TABLE_H__
#define __ALLOC_TABLE_H__

typedef struct
{
    uint64_t index;           // 该条目的索引
    uint64_t number_of_units; // 该条目包含几个空置单元
} allocate_table_entry_t;

typedef struct
{
    uint64_t                number_of_entries; // 条目总数
    uint64_t                frees;             // 空闲条目数
    allocate_table_entry_t *entries;           // 条目内容
} allocate_table_t;

PUBLIC void allocate_table_init(allocate_table_t *table,
                                allocate_table_entry_t *entries,
                                uint64_t number_of_entries);
PUBLIC status_t allocate_units
(
    IN(allocate_table_t *table,uint64_t number_of_units),
    OUT(uint64_t *index)
);

PUBLIC void free_units(allocate_table_t *table,uint64_t index,
                       uint64_t number_of_units);
PUBLIC uint64_t total_free_units(allocate_table_t *table);

#endif