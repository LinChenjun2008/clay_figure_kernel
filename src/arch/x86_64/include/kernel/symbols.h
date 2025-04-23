#ifndef __SYMBOLS_H__
#define __SYMBOLS_H__

PUBLIC int is_available_symbol_address(void *addr);
PUBLIC status_t get_symbol_index_by_addr(void *addr,int *index);
PUBLIC const char* index_to_symbol(int index);
PUBLIC void* index_to_addr(int index);
PUBLIC const char* addr_to_symbol(void *addr);

#endif