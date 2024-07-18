#ifndef __XHCI_H__
#define __XHCI_H__

#pragma pack(1)

typedef struct
{
   volatile uint8_t  CAPLENGTH;
   volatile uint8_t  Rsvd;
   volatile uint16_t HCIVERSION;
   volatile uint32_t HCSPARAMS1;
   volatile uint32_t HCSPARAMS2;
   volatile uint32_t HCSPARAMS3;
   volatile uint32_t HCCPARAMS1;
   volatile uint32_t DBOFF;
   volatile uint32_t RTSOFF;
   volatile uint32_t HCCPARAMS2;
    // (CAPLENGTH - 0x20) -- Rsvd
} xhci_cap_regs_t;

// HCSPARAMS1
#define MAX_SLOTS 0x000000ff

// RTSOFF
#define OFFSET    0xffffffe0

typedef struct
{
   volatile uint32_t USBCMD;
   volatile uint32_t USBSTS;
   volatile uint32_t PAGESIZE;
   volatile uint32_t RsvdZ_1[2];
   volatile uint32_t DNCTRL;
   volatile uint64_t CRCR;
   volatile uint32_t RsvdZ_2[4];
   volatile uint64_t DCBAAP;
   volatile uint32_t CONFIG;
} xhci_opt_regs_t;

// USBCMD
#define RUN_STOP (1 <<  0)
#define HCRST    (1 <<  1)
#define INTE     (1 <<  2)
#define HSEE     (1 <<  3)

#define EWE      (1 << 10)

// USBSTS
#define HCH      (1 <<  0)

#define CNR      (1 << 11)

// CRCR
#define RCS      (1ULL <<  0)
#define CS       (1ULL <<  1)
#define CA       (1ULL <<  2)
#define CRP      (~0x3fULL)

typedef struct
{
    uint32_t PORTSC;
    uint32_t PORTPMSC;
    uint32_t PORTLI;
    uint32_t Rsvd;
} xhci_usb_port_regs_t;

// PORTSC
#define CCS      (  1 <<  0)
#define PED      (  1 <<  1)
#define PR       (  1 <<  4)
#define PLS      (0xf <<  5)
#define CSC      ( 1  << 17)

typedef struct
{
    uint32_t data[4];
    uint32_t RsvdO[4];
} slot_context_t;

typedef struct
{
    uint32_t data[5];
    uint32_t RsvdO[3];
} endpoint_context_t;

typedef struct
{
    uint32_t IMAN;
    uint32_t IMOD;
    uint32_t ERSTSZ;
    uint32_t reserved;
    uint64_t ERSTBA;
    uint64_t ERDP;
} intr_reg_set_t;

typedef struct
{
    slot_context_t     slot_ctx;
    endpoint_context_t endpoint_ctx[31];
} xhci_dev_context_t;

typedef struct
{
    uint32_t data[4];
} trb_t;

typedef struct
{
    trb_t  *trb;
    size_t  trb_size;
    uint8_t cycle_bit;
    size_t  write_index;
} ring_t;

typedef struct
{
    uint64_t rsba; // low 6 bit reserved.
    uint32_t data[2];
} erst_t;

typedef struct
{
    trb_t          *trb;
    size_t          trb_size;
    uint8_t         cycle_bit;
    erst_t         *erst;
    intr_reg_set_t *intr_reg_sets;
} event_ring_t;

#pragma pack()

typedef struct
{
    void                *mmio_base;
    xhci_cap_regs_t     *cap_regs;
    xhci_opt_regs_t     *opt_regs;
    uint8_t              max_ports;
    uint8_t              max_slots;
    xhci_dev_context_t **dev_cxt;
    intr_reg_set_t      *intr_reg_sets;
    size_t               intr_reg_sets_size;

    ring_t               cr;
    event_ring_t         er;

} xhci_t;

PUBLIC void xhci_init();

#endif