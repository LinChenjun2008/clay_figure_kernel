/*
   Copyright 2024-2025 LinChenjun

   本程序是自由软件
   修改和/或再分发依照 GNU GPL version 3 (or any later version)

*/

#ifndef __XHCI_H__
#define __XHCI_H__

#include <device/usb/xhci_ring.h> // xhci_ring_t

#pragma pack(1)

// Section 6.2.2
typedef struct xhci_slot_ctx_s
{
    uint32_t slot0;
    uint32_t slot1;
    uint32_t slot2;
    uint32_t slot3;
    uint32_t RsvdO[4];
} xhci_slot_ctx_t;

// Section 6.2.3
typedef struct xhci_endpoint_ctx_s
{
    uint32_t endpoint0;
    uint32_t endpoint1;
    uint64_t endpoint2;
    uint32_t endpoint4;
    uint32_t RsvdO[3];
} xhci_endpoint_ctx_t;

// // Section 6.2.1
// typedef struct xhci_device_ctx_s
// {
//     xhci_slot_ctx_t slot;
//     xhci_endpoint_ctx_t endpoint[XHCI_MAX_ENDPOINTS - 1];
// } xhci_device_ctx_t;


#pragma pack()

// typedef struct xhci_device_cxt_arr_s
// {
//     uint64_t base_addr[XHCI_MAX_SLOTS];
//     uint64_t padding;
//     uint64_t scratchpad[XHCI_MAX_SCRATCHPADS];
// } xhci_device_cxt_arr_t;


typedef struct xhci_portmap_s
{
    uint8_t start;
    uint8_t count;
} xhci_portmap_t;

typedef struct xhci_device_s
{
    uint8_t port_id; // 1-based
    uint8_t slot_id; // slots index in DCBAA
    uint8_t speed;   // Speed
    uint8_t using_64byte_csz; // 64-byte context size
    void   *input_cxt;
    addr_t  dma_input_cxt;
} xhci_device_t;

typedef struct xhci_s
{
    uint8_t               *mmio_base;
    uint8_t               *cap_regs;
    uint8_t               *opt_regs;
    uint8_t               *run_regs;
    uint8_t               *doorbell_regs;

    // Capability Register
    // caplength
    uint8_t                cap_len;

    // hcsparam1
    uint8_t                max_slots;
    uint8_t                max_intr;
    uint8_t                max_ports;

    // hcsparam2
    uint8_t                isoc_sched_thre; // Isochronous Scheduling Threshold
    uint8_t                erst_max;
    uint8_t                max_scratchpad_buffer;

    // hccparam1
    uint8_t                ac64; // 64-bit addressing capibility
    uint8_t                bnc;  // bandwidth_negotiation_capability
    uint8_t                csz;  // 64-byte context size
    uint8_t                ppc;  // port power control
    uint8_t                pind; // port indicators
    uint8_t                lhrc; // light reset capability

    uint32_t               xecp_offset;

    // Operational Registers
    //
    uint64_t               pagesize;

    xhci_portmap_t         usb2;
    xhci_portmap_t         usb3;

    xhci_device_t        **connected_devices; // size = max_slots

    // Device Context Base Address Array's virtual address
    // dcbaa[i] = scratchpad buffer physical address
    uint64_t              *dcbaa;
    // dcbaa_vaddr[i] = scratchpad buffer virtual address
    uint64_t              *dcbaa_vaddr;

    xhci_command_ring_t    command_ring;
    xhci_event_ring_t      event_ring;
    // xhci_erst_t           *erst;
    // xhci_device_cxt_arr_t *dev_cxt_arr;

    // uint8_t                max_address;
} xhci_t;

/**
 * @brief 对所有xHCI进行初始化
 */
PUBLIC status_t xhci_setup(void);

/**
 * @brief 向xHCI发出命令
 * @param xhci xhci结构体指针
 * @param trb 命令的TRB结构体指针
 * @return 成功将返回K_SUCCESS,若失败则返回对应的错误码
 */
// PUBLIC status_t xhci_submit_command(xhci_t *xhci,xhci_trb_t *trb);

/**
 * @brief 对xHCI进行初始化,并启用xHCI
 * @param xhci 将要被初始化的xhci控制器结构体指针
 * @return 成功将返回K_SUCCESS,若失败则返回对应的错误码
 */
PUBLIC status_t xhci_init(xhci_t *xhci);

/**
 * @brief 判断xHCI Ring是否空闲
 * @param ring 将判断这个ring是否空闲
 * @return 空闲将返回0,否则返回1
 */
// PUBLIC bool xhci_ring_busy(xhci_ring_t *ring);



// registers.c

/**
 * @brief 读取xHCI能力寄存器(Capability Registers)
 * @param xhci xHCI结构体指针
 * @param reg 寄存器偏移地址
 * @return 寄存器值
 */
PUBLIC uint32_t xhci_read_cap(xhci_t *xhci,uint32_t reg);

/**
 * @brief 读取xHCI操作寄存器(Operational Registers)
 * @param xhci xHCI结构体指针
 * @param reg 寄存器偏移地址
 * @return 寄存器值
 */
PUBLIC uint32_t xhci_read_opt(xhci_t *xhci,uint32_t reg);

/**
 * @brief 写入xHCI操作寄存器(Operational Registers)
 * @param xhci xHCI结构体指针
 * @param reg 寄存器偏移地址
 * @param val 写入值
 */
PUBLIC void xhci_write_opt(xhci_t *xhci,uint32_t reg,uint32_t val);

/**
 * @brief 读取xHCI运行时寄存器(Runtine Registers)
 * @param xhci xHCI结构体指针
 * @param reg 寄存器偏移地址
 * @return 寄存器值
 */
PUBLIC uint32_t xhci_read_run(xhci_t *xhci,uint32_t reg);

/**
 * @brief 写入xHCI运行时寄存器(Runtine Registers)
 * @param xhci xHCI结构体指针
 * @param reg 寄存器偏移地址
 * @param value 写入值
 */
PUBLIC void xhci_write_run(xhci_t *xhci,uint32_t reg,uint32_t val);

/**
 * @brief 读取xHCI门铃寄存器(Doorbell Registers)
 * @param xhci xHCI结构体指针
 * @param reg 寄存器偏移地址
 * @return 寄存器值
 */
PUBLIC uint32_t xhci_read_doorbell(xhci_t *xhci,uint32_t reg);

/**
 * @brief 写入xHCI门铃寄存器(Doorbell Registers)
 * @param xhci xHCI结构体指针
 * @param reg 寄存器偏移地址
 * @param value 写入值
 */
PUBLIC void xhci_write_doorbell(xhci_t *xhci,uint32_t reg,uint32_t val);

#endif