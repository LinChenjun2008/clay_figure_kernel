/*
   Copyright 2024-2025 LinChenjun

   本程序是自由软件
   修改和/或再分发依照 GNU GPL version 3 (or any later version)

*/

#ifndef __XHCI_H__
#define __XHCI_H__

#include <device/usb/xhci_ring.h>   // xhci_ring_t
#include <device/usb/xhci_device.h> // xhci_device_t
#include <lib/fifo.h>               // fifo_t

#define XHCI_USB_SPEED_UNDEFINED            0
#define XHCI_USB_SPEED_FULL_SPEED           1 // 12 MB/s USB 2.0
#define XHCI_USB_SPEED_LOW_SPEED            2 // 1.5 Mb/s USB 2.0
#define XHCI_USB_SPEED_HIGH_SPEED           3 // 480 Mb/s USB 2.0
#define XHCI_USB_SPEED_SUPER_SPEED          4 // 5 Gb/s (Gen1 x1) USB 3.0
#define XHCI_USB_SPEED_SUPER_SPEED_PLUS     5 // 10 Gb/s (Gen2 x1) USB 3.1

typedef struct xhci_portmap_s
{
    uint8_t start;
    uint8_t count;
} xhci_portmap_t;

typedef struct xhci_port_connection_event_s
{
    uint8_t port_id;
    uint8_t is_connected;
} xhci_port_connection_event_t;

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
    // dcbaa[0] = scratchpad buffer physical address
    uint64_t              *dcbaa;
    // dcbaa_vaddr[0] = scratchpad buffer virtual address
    uint64_t              *dcbaa_vaddr;

    xhci_command_ring_t    command_ring;
    xhci_event_ring_t      event_ring;

    fifo_t                 port_connection_events;
    fifo_t                 port_status_change_events;
    fifo_t                 command_completion_events;
    fifo_t                 transfer_completion_events;
} xhci_t;

/**
 * @brief 对所有xHCI进行初始化
 */
PUBLIC status_t xhci_setup(void);

/**
 * @brief 启动xHCI控制器
 */
PUBLIC status_t xhci_start(void);

/**
 * @brief 向xHCI发出命令
 * @param xhci xhci结构体指针
 * @param trb 命令的TRB结构体指针,执行成功后为对应的COMMAND_COMPLETION_EVENT trb
 * @return 成功将返回K_SUCCESS,若失败则返回对应的错误码
 */
PUBLIC status_t xhci_submit_command(xhci_t *xhci,xhci_trb_t *trb);

PUBLIC status_t xhci_start_ctrl_ep_transfer(
    xhci_t *xhci,
    xhci_transfer_ring_t *ring,
    xhci_trb_t *trb);

#endif