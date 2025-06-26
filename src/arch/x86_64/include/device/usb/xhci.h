/*
   Copyright 2024-2025 LinChenjun

   本程序是自由软件
   修改和/或再分发依照 GNU GPLv3-or-later

*/

#ifndef __XHCI_H__
#define __XHCI_H__

#include <device/usb/usb.h> // usb_t, usb_pipe_t, usb_hub_set_t
#include <lib/fifo.h>       // fifo_t

// registers

/* XHCI Capability Regisers (Secton 5.3)
offset |  size (byte)| Mnemonic
   00h |           1 | CAPLENGTH
   01h |           1 | Rsvd
   02h |           2 | HCIVERSION
   04h |           4 | HCSPARAMS 1
   08h |           4 | HCSPARAMS 2
   0Ch |           4 | HCSPARAMS 3
   10h |           4 | HCCPARAM  1
   14h |           4 | DBOFF
   18h |           4 | RTSOFF
   20h |           4 | HCCPARAM  2
   20h |CAPLENGTH-20h| Rsvd
*/

#define XHCI_CAP_CAPLENGTH  0x00 /* bits  7:0 */
#define XHCI_CAP_HCIVERSION 0x00 /* bits 31:16*/
#define XHCI_CAP_HCSPARAM1  0x04
#define XHCI_CAP_HCSPARAM2  0x08
#define XHCI_CAP_HCSPARAM3  0x0c
#define XHCI_CAP_HCCPARAM1  0x10
#define XHCI_CAP_DBOFF      0x14
#define XHCI_CAP_RSTOFF     0x18
#define XHCI_CAP_HCCPARAM2  0x1c

// CAPLENGTH
#define CAPLENGTH_SHIFT 0
#define CAPLENGTH_MASK  0x00ff

// HCIVERSION
#define HCIVERSION_SHIFT 16
#define HCIVERSION_MASK  0xffff

// HCSPARAM1
#define HCSP1_MAX_SLOTS_SHIFT 0
#define HCSP1_MAX_SLOTS_MASK  0x0ff

#define HCSP1_MAX_INTR_SHIFT 8
#define HCSP1_MAX_INTR_MASK  0x3ff

#define HCSP1_MAX_PORTS_SHIFT 24
#define HCSP1_MAX_PORTS_MASK  0x0ff

// HCSPARAM2
#define HCSP2_IST_SHIFT 0
#define HCSP2_IST_MASK  0x07

#define HCSP2_ERST_MAX_SHIFT 3
#define HCSP2_ERST_MAX_MASK  0x0f

#define HCSP2_SPR_SHIFT 26
#define HCSP2_SPR_MASK  0x01

#define HCSP2_MAX_SC_BUF_LO_SHIFT 27
#define HCSP2_MAX_SC_BUF_LO_MASK  0x1f

#define HCSP2_MAX_SC_BUF_HI_SHIFT 21
#define HCSP2_MAX_SC_BUF_HI_MASK  0x1f

#define GET_HCSP2_MAX_SC_BUF(x)                 \
    ((GET_FIELD(x, HCSP2_MAX_SC_BUF_HI) << 5) | \
     GET_FIELD(x, HCSP2_MAX_SC_BUF_LO))

// HCSPARAM3
#define HCSP3_U1_DEVICE_LATENCY_SHIFT 0
#define HCSP3_U1_DEVICE_LATENCY_MASK  0xff

#define HCSP3_U2_DEVICE_LATENCY_SHIFT 16
#define HCSP3_U2_DEVICE_LATENCY_MASK  0xff

// HCCPARAM1
#define HCCP1_AC64_SHIFT 0
#define HCCP1_AC64_MASK  0x001

#define HCCP1_BNC_SHIFT 1
#define HCCP1_BNC_MASK  0x001

#define HCCP1_CSZ_SHIFT 2
#define HCCP1_CSZ_MASK  0x001

#define HCCP1_PPC_SHIFT 3
#define HCCP1_PPC_MASK  0x001

#define HCCP1_PIND_SHIFT 4
#define HCCP1_PIND_MASK  0x001

#define HCCP1_LHRC_SHIFT 5
#define HCCP1_LHRC_MASK  0x001

#define HCCP1_LTC_SHIFT 6
#define HCCP1_LTC_MASK  0x001

#define HCCP1_NSS_SHIFT 7
#define HCCP1_NSS_MASK  0x001

#define HCCP1_PAE_SHIFT 8
#define HCCP1_PAE_MASK  0x001

#define HCCP1_SPC_SHIFT 9
#define HCCP1_SPC_MASK  0x001

#define HCCP1_SEC_SHIFT 10
#define HCCP1_SEC_MASK  0x001

#define HCCP1_CFC_SHIFT 11
#define HCCP1_CFC_MASK  0x001

#define HCCP1_MAXPSASIZE_SHIFT 12
#define HCCP1_MAXPSASIZE_MASK  0x00f

#define HCCP1_XECP_SHIFT 16
#define HCCP1_XECP_MASK  0xfff



// XHCI Extended Capabilities
#define XECP_CAP_ID_SHIFT 0
#define XECP_CAP_ID_MASK  0xff

#define XECP_NEXT_POINT_SHIFT 8
#define XECP_NEXT_POINT_MASK  0xff

#define XECP_CAP_SPEC_SHIFT 16
#define XECP_CAP_SPEC_MASK  0xffff

// xHCI Supported Protocol Capability
#define XECP_SUP_MAJOR_SHIFT 24
#define XECP_SUP_MAJOR_MASK  0xff

#define XECP_SUP_MINOR_SHIFT 16
#define XECP_SUP_MINOR_MASK  0xff

typedef struct xhci_xcap_regs_s
{
    uint32_t cap;
    uint32_t data[];
} xhci_xcap_regs_t;

/* XHCI Operational Registers (Section 5.4)
00h       USBCMD
04h       USBSTS
08h       PAGESIZE
0C-13h    RsvdZ
14h       DNCTRL
18h       CRCR
20-2Fh    RsvdZ
30h       DCBAAP
38h       CONFIG
3C-3FFh   RsvdZ
400-13FFh Port Register Set 1-MaxPorts
*/

#define XHCI_OPT_USBCMD    0x00
#define XHCI_OPT_USBSTS    0x04
#define XHCI_OPT_PAGESIZE  0x08
#define XHCI_OPT_DNCTRL    0x14
#define XHCI_OPT_CRCR_LO   0x18
#define XHCI_OPT_CRCR_HI   0x1c
#define XHCI_OPT_DCBAAP_LO 0x30
#define XHCI_OPT_DCBAAP_HI 0x34
#define XHCI_OPT_CONFIG    0x38

#define XHCI_OPT_PORTSC(n) \
    (0x400 + ((n) * 0x10)) // n: Port Number-1 (0,1,2,...,Max Port-1)
#define XHCI_OPT_PORTPMSC(n) \
    (0x404 + ((n) * 0x10)) // n: Port Number-1 (0,1,2,...,Max Port-1)
#define XHCI_OPT_PORTLI(n) \
    (0x408 + ((n) * 0x10)) // n: Port Number-1 (0,1,2,...,Max Port-1)
#define XHCI_OPT_PORTHLPMC(n) \
    (0x40c + ((n) * 0x10)) // n: Port Number-1 (0,1,2,...,Max Port-1)

/* XHCI Runtime Regiseters
     Offset | Mnemonic
      0000h | MFINDEX Microframe Index
0004h:0020h | RsvdZ
      0020h | IR0 Interrupter Register Set 0
      ....  | ....
      8000h | IR1023 Interrupter Register Set 1023
*/

#define XHCI_RUN_MFINDEX 0x00

/* Interrupt Regisets
Offset | Size (bits) | Mnemonic
00h    |          32 | IMAN
04h    |          32 | IMOD
08h    |          32 | ERSTSZ
0Ch    |          32 | RsvdP
10h    |          64 | ERSTBA
18h    |          64 | ERDP
*/

#define XHCI_IRS_IMAN(n)      (0x0020 + (0x20 * (n)))
#define XHCI_IRS_IMOD(n)      (0x0024 + (0x20 * (n)))
#define XHCI_IRS_ERSTSZ(n)    (0x0028 + (0x20 * (n)))
#define XHCI_IRS_ERSTBA_LO(n) (0x0030 + (0x20 * (n)))
#define XHCI_IRS_ERSTBA_HI(n) (0x0034 + (0x20 * (n)))
#define XHCI_IRS_ERDP_LO(n)   (0x0038 + (0x20 * (n)))
#define XHCI_IRS_ERDP_HI(n)   (0x003C + (0x20 * (n)))

// IMAN
#define IMAN_INTR_PENDING (1 << 0)
#define IMAN_INTR_ENABLE  (1 << 1)

// ERSTSZ
#define ERSTSZ_SET(n) ((n) & 0xffff)

// ERDP
#define ERDP_EHB(x) (((x) >> 3) & 0x01)

// Doorbell Regs
#define XHCI_DOORBELL(n)          ((n) * 4)
#define XHCI_DOORBELL_TARGET(x)   ((x) & 0xff)
#define XHCI_DOORBELL_STREAMID(x) (((x) & 0xffff) << 16)

// --------------------------------------------------------------
// configuration

#define XHCI_PAGE_SIZE 4096

#define XHCI_RING_ITEMS 16
#define XHCI_RING_SIZE  (XHCI_RING_ITEMS * sizeof(xhci_trb_t))

/*
 *  xhci_ring structs are allocated with XHCI_RING_SIZE alignment,
 *  then we can get it from a trb pointer (provided by evt ring).
 */
#define XHCI_RING(_trb) \
    ((struct xhci_ring *)((uint32_t)(_trb) & ~(XHCI_RING_SIZE - 1)))

// --------------------------------------------------------------
// bit definitions

#define XHCI_CMD_RS     (1 << 0)
#define XHCI_CMD_HCRST  (1 << 1)
#define XHCI_CMD_INTE   (1 << 2)
#define XHCI_CMD_HSEE   (1 << 3)
#define XHCI_CMD_LHCRST (1 << 7)
#define XHCI_CMD_CSS    (1 << 8)
#define XHCI_CMD_CRS    (1 << 9)
#define XHCI_CMD_EWE    (1 << 10)
#define XHCI_CMD_EU3S   (1 << 11)

#define XHCI_STS_HCH  (1 << 0)
#define XHCI_STS_HSE  (1 << 2)
#define XHCI_STS_EINT (1 << 3)
#define XHCI_STS_PCD  (1 << 4)
#define XHCI_STS_SSS  (1 << 8)
#define XHCI_STS_RSS  (1 << 9)
#define XHCI_STS_SRE  (1 << 10)
#define XHCI_STS_CNR  (1 << 11)
#define XHCI_STS_HCE  (1 << 12)

#define XHCI_PORTSC_CCS         (1 << 0)
#define XHCI_PORTSC_PED         (1 << 1)
#define XHCI_PORTSC_OCA         (1 << 3)
#define XHCI_PORTSC_PR          (1 << 4)
#define XHCI_PORTSC_PLS_SHIFT   5
#define XHCI_PORTSC_PLS_MASK    0x0f
#define XHCI_PORTSC_PP          (1 << 9)
#define XHCI_PORTSC_SPEED_SHIFT 10
#define XHCI_PORTSC_SPEED_MASK  0x0f
#define XHCI_PORTSC_SPEED_FULL  (1 << 10)
#define XHCI_PORTSC_SPEED_LOW   (2 << 10)
#define XHCI_PORTSC_SPEED_HIGH  (3 << 10)
#define XHCI_PORTSC_SPEED_SUPER (4 << 10)
#define XHCI_PORTSC_PIC_SHIFT   14
#define XHCI_PORTSC_PIC_MASK    0x03
#define XHCI_PORTSC_LWS         (1 << 16)
#define XHCI_PORTSC_CSC         (1 << 17)
#define XHCI_PORTSC_PEC         (1 << 18)
#define XHCI_PORTSC_WRC         (1 << 19)
#define XHCI_PORTSC_OCC         (1 << 20)
#define XHCI_PORTSC_PRC         (1 << 21)
#define XHCI_PORTSC_PLC         (1 << 22)
#define XHCI_PORTSC_CEC         (1 << 23)
#define XHCI_PORTSC_CAS         (1 << 24)
#define XHCI_PORTSC_WCE         (1 << 25)
#define XHCI_PORTSC_WDE         (1 << 26)
#define XHCI_PORTSC_WOE         (1 << 27)
#define XHCI_PORTSC_DR          (1 << 30)
#define XHCI_PORTSC_WPR         (1 << 31)

#define XHCI_TIME_POSTPOWER 20

#define TRB_C          (1 << 0)
#define TRB_TYPE_SHIFT 10
#define TRB_TYPE_MASK  0x3f
#define TRB_TYPE(t)    (((t) >> TRB_TYPE_SHIFT) & TRB_TYPE_MASK)

#define TRB_EV_ED (1 << 2)

#define TRB_TR_ENT           (1 << 1)
#define TRB_TR_ISP           (1 << 2)
#define TRB_TR_NS            (1 << 3)
#define TRB_TR_CH            (1 << 4)
#define TRB_TR_IOC           (1 << 5)
#define TRB_TR_IDT           (1 << 6)
#define TRB_TR_TBC_SHIFT     7
#define TRB_TR_TBC_MASK      0x03
#define TRB_TR_BEI           (1 << 9)
#define TRB_TR_TLBPC_SHIFT   16
#define TRB_TR_TLBPC_MASK    0x0f
#define TRB_TR_FRAMEID_SHIFT 20
#define TRB_TR_FRAMEID_MASK  0x7ff
#define TRB_TR_SIA           (1 << 31)

#define TRB_TR_DIR (1 << 16)

#define TRB_CR_SLOTID_SHIFT 24
#define TRB_CR_SLOTID_MASK  0xff
#define TRB_CR_EPID_SHIFT   16
#define TRB_CR_EPID_MASK    0x1f

#define TRB_CR_BSR (1 << 9)
#define TRB_CR_DC  (1 << 9)

#define TRB_LK_TC (1 << 1)

#define TRB_INTR_SHIFT 22
#define TRB_INTR_MASK  0x3ff
#define TRB_INTR(t)    (((t).status >> TRB_INTR_SHIFT) & TRB_INTR_MASK)

typedef enum trb_type_e
{
    TRB_RESERVED = 0,
    TR_NORMAL,
    TR_SETUP,
    TR_DATA,
    TR_STATUS,
    TR_ISOCH,
    TR_LINK,
    TR_EVDATA,
    TR_NOOP,
    CR_ENABLE_SLOT,
    CR_DISABLE_SLOT,
    CR_ADDRESS_DEVICE,
    CR_CONFIGURE_ENDPOINT,
    CR_EVALUATE_CONTEXT,
    CR_RESET_ENDPOINT,
    CR_STOP_ENDPOINT,
    CR_SET_TR_DEQUEUE,
    CR_RESET_DEVICE,
    CR_FORCE_EVENT,
    CR_NEGOTIATE_BW,
    CR_SET_LATENCY_TOLERANCE,
    CR_GET_PORT_BANDWIDTH,
    CR_FORCE_HEADER,
    CR_NOOP,
    ER_TRANSFER = 32,
    ER_COMMAND_COMPLETE,
    ER_PORT_STATUS_CHANGE,
    ER_BANDWIDTH_REQUEST,
    ER_DOORBELL,
    ER_HOST_CONTROLLER,
    ER_DEVICE_NOTIFICATION,
    ER_MFINDEX_WRAP,
} trb_type_t;

typedef enum trb_comp_code_e
{
    CC_INVALID = 0,
    CC_SUCCESS,
    CC_DATA_BUFFER_ERROR,
    CC_BABBLE_DETECTED,
    CC_USB_TRANSACTION_ERROR,
    CC_TRB_ERROR,
    CC_STALL_ERROR,
    CC_RESOURCE_ERROR,
    CC_BANDWIDTH_ERROR,
    CC_NO_SLOTS_ERROR,
    CC_INVALID_STREAM_TYPE_ERROR,
    CC_SLOT_NOT_ENABLED_ERROR,
    CC_EP_NOT_ENABLED_ERROR,
    CC_SHORT_PACKET,
    CC_RING_UNDERRUN,
    CC_RING_OVERRUN,
    CC_VF_ER_FULL,
    CC_PARAMETER_ERROR,
    CC_BANDWIDTH_OVERRUN,
    CC_CONTEXT_STATE_ERROR,
    CC_NO_PING_RESPONSE_ERROR,
    CC_EVENT_RING_FULL_ERROR,
    CC_INCOMPATIBLE_DEVICE_ERROR,
    CC_MISSED_SERVICE_ERROR,
    CC_COMMAND_RING_STOPPED,
    CC_COMMAND_ABORTED,
    CC_STOPPED,
    CC_STOPPED_LENGTH_INVALID,
    CC_MAX_EXIT_LATENCY_TOO_LARGE_ERROR = 29,
    CC_ISOCH_BUFFER_OVERRUN             = 31,
    CC_EVENT_LOST_ERROR,
    CC_UNDEFINED_ERROR,
    CC_INVALID_STREAM_ID_ERROR,
    CC_SECONDARY_BANDWIDTH_ERROR,
    CC_SPLIT_TRANSACTION_ERROR
} trb_comp_code_t;

enum
{
    PLS_U0              = 0,
    PLS_U1              = 1,
    PLS_U2              = 2,
    PLS_U3              = 3,
    PLS_DISABLED        = 4,
    PLS_RX_DETECT       = 5,
    PLS_INACTIVE        = 6,
    PLS_POLLING         = 7,
    PLS_RECOVERY        = 8,
    PLS_HOT_RESET       = 9,
    PLS_COMPILANCE_MODE = 10,
    PLS_TEST_MODE       = 11,
    PLS_RESUME          = 15,
};

// --------------------------------------------------------------
// memory data structs

#pragma pack(1)

// slot context
typedef struct xhci_slot_ctx_s
{
    uint32_t ctx[4];
    uint32_t reserved_01[4];
} xhci_slot_ctx_t;

// endpoint context
typedef struct xhci_ep_ctx_s
{
    uint32_t ctx[2];
    uint64_t deq_ptr;
    uint32_t length;
    uint32_t reserved_01[3];
} xhci_ep_ctx_t;

// device context array element
typedef struct xhci_dev_list_s
{
    uint64_t ptr;
} xhci_dev_list_t;

// input context
typedef struct xhci_in_ctx_s
{
    uint32_t del;
    uint32_t add;
    uint32_t reserved_01[6];
} xhci_in_ctx_t;

// transfer block (ring element)
typedef struct xhci_trb_s
{
    uint64_t ptr;
    uint32_t status;
    uint32_t control;
} xhci_trb_t;

// event ring segment
typedef struct xhci_er_seg_s
{
    uint32_t ptr_lo;
    uint32_t ptr_hi;
    uint32_t size;
    uint32_t reserved_01;
} xhci_er_seg_t;

#pragma pack()

// --------------------------------------------------------------
// state structs

typedef struct xhci_ring_s
{
    xhci_trb_t *trbs;
    size_t      trb_count;
    uint32_t    enqueue;
    uint32_t    dequeue;
    uint32_t    cs;
    // spinlock_t      lock;
} xhci_ring_t;

typedef struct xhci_portmap_s
{
    uint8_t start;
    uint8_t count;
} xhci_portmap_t;

typedef struct xhci_s
{
    usb_t usb;

    /* xhci registers */
    uint8_t *mmio_base;
    uint8_t *cap_regs;
    uint8_t *opt_regs;
    uint8_t *run_regs;
    uint8_t *doorbell_regs;

    /* devinfo */
    uint32_t       xecp;
    uint32_t       ports;
    uint32_t       slots;
    uint8_t        context64;
    xhci_portmap_t usb2;
    xhci_portmap_t usb3;

    /* xhci data structures */
    xhci_dev_list_t *devs;
    xhci_er_seg_t   *eseg;
    xhci_ring_t      cmds;
    xhci_ring_t      evts;

    /* events */
    fifo_t cmds_evts; // command completion event
    fifo_t port_evts; // port status change event
    fifo_t xfer_evts; // transfer completion event
} xhci_t;

typedef struct xhci_pipe_s
{
    xhci_ring_t reqs;

    usb_pipe_t pipe;
    uint32_t   slot_id;
    uint32_t   epid;
    void      *buf;
    int        bufused;
} xhci_pipe_t;

/**
 * @brief 对所有xHCI进行初始化
 * @param hub_set
 */
PUBLIC status_t xhci_setup(usb_hub_set_t *hub_set);

/**
 * @brief
 * @param xhci
 * @return
 */
PUBLIC void xhci_process_events(xhci_t *xhci);

PUBLIC usb_pipe_t *xhci_realloc_pipe(
    usb_device_t              *usb_dev,
    usb_pipe_t                *upipe,
    usb_endpoint_descriptor_t *epdesc
);


PUBLIC int xhci_send_pipe(
    usb_pipe_t *pipe,
    int         dir,
    const void *cmd,
    void       *data,
    int         data_len
);

/**
 * @brief 读取xHCI能力寄存器(Capability Registers)
 * @param xhci xHCI结构体指针
 * @param reg 寄存器偏移地址
 * @return 寄存器值
 */
PUBLIC uint32_t xhci_read_cap(xhci_t *xhci, uint32_t reg);

/**
 * @brief 读取xHCI操作寄存器(Operational Registers)
 * @param xhci xHCI结构体指针
 * @param reg 寄存器偏移地址
 * @return 寄存器值
 */
PUBLIC uint32_t xhci_read_opt(xhci_t *xhci, uint32_t reg);

/**
 * @brief 写入xHCI操作寄存器(Operational Registers)
 * @param xhci xHCI结构体指针
 * @param reg 寄存器偏移地址
 * @param val 写入值
 */
PUBLIC void xhci_write_opt(xhci_t *xhci, uint32_t reg, uint32_t val);

/**
 * @brief 读取xHCI运行时寄存器(Runtine Registers)
 * @param xhci xHCI结构体指针
 * @param reg 寄存器偏移地址
 * @return 寄存器值
 */
PUBLIC uint32_t xhci_read_run(xhci_t *xhci, uint32_t reg);

/**
 * @brief 写入xHCI运行时寄存器(Runtine Registers)
 * @param xhci xHCI结构体指针
 * @param reg 寄存器偏移地址
 * @param value 写入值
 */
PUBLIC void xhci_write_run(xhci_t *xhci, uint32_t reg, uint32_t val);

/**
 * @brief 读取xHCI门铃寄存器(Doorbell Registers)
 * @param xhci xHCI结构体指针
 * @param reg 寄存器偏移地址
 * @return 寄存器值
 */
PUBLIC uint32_t xhci_read_doorbell(xhci_t *xhci, uint32_t reg);

/**
 * @brief 写入xHCI门铃寄存器(Doorbell Registers)
 * @param xhci xHCI结构体指针
 * @param reg 寄存器偏移地址
 * @param value 写入值
 */
PUBLIC void xhci_write_doorbell(xhci_t *xhci, uint32_t reg, uint32_t val);

#endif