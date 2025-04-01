/*
   Copyright 2024-2025 LinChenjun

   本程序是自由软件
   修改和/或再分发依照 GNU GPL version 3 (or any later version)

*/

#ifndef __IO_H__
#define __IO_H__

extern void io_cli(void);
extern void io_sti(void);
extern void io_hlt(void);
extern void io_stihlt(void);
extern void io_mfence(void);

extern uint32_t io_in8(uint32_t port);
extern uint32_t io_in16(uint32_t port);
extern uint32_t io_in32(uint32_t port);
extern void io_out8(uint32_t port,uint32_t data);
extern void io_out16(uint32_t port,uint32_t data);
extern void io_out32(uint32_t port,uint32_t data);

extern wordsize_t get_flags(void);

extern wordsize_t get_rsp();
extern wordsize_t get_cr2();
extern wordsize_t get_cr3();
extern void set_cr3(wordsize_t cr3);
#endif