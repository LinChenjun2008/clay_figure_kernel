#ifndef __XHCI_H__
#define __XHCI_H__

/* XHCI Capability Regisers (Secton 5.3)
offset |  size (byte)| Mnemonic
   00h |           1 | CAPLENGTH
   01h |           1 | Rsvd
   02h |           2 | HCIVERSION
   04h |           4 | HCSPARAMS 1
   08h |           4 | HCSPARAMS 2
   0Ch |           4 | HCSPARAMS 3
   10h |           4 | HCCPARAMS
   14h |           4 | DBOFF
   18h |           4 | RTSOFF
   1Ch |CAPLENGTH-1Ch| Rsvd
*/

#define XHCI_CAP_REG_CAPLENGTH  0x00 /* bits  7:0 */
#define XHCI_CAP_REG_HCIVERSION 0x00 /* bits 31:16*/
#define XHCI_CAP_REG_HCSPARAM1  0x04
#define XHCI_CAP_REG_HCSPARAM2  0x08
#define XHCI_CAP_REG_HCSPARAM3  0x0c
#define XHCI_CAP_REG_HCCPARAMS  0x10
#define XHCI_CAP_REG_DBOFF      0x14
#define XHCI_CAP_REG_RSTOFF     0x18

// CAPLENGTH
#define CAPLENGTH(x)  (((x) >>  0) & 0x00ff)

// HCIVERSION
#define HCIVERSION(x) (((x) >> 16) & 0xffff)

// HCSPARAM1
#define HCSP1_MAX_SLOTS(x) (((x) >>  0) & 0x0ff)
#define HCSP1_MAX_INTR(x)  (((x) >>  8) & 0x3ff)
#define HCSP1_MAX_PORTS(x) (((x) >> 24) & 0x0ff)

// HCSPARAM2
#define HCSP2_IST(x)        (((x) >>  0) & 0x07)
#define HCSP2_ERST_MAX(x)   (((x) >>  3) & 0x0f)
#define HCSP2_SPR(x)        (((x) >> 26) & 0x01)
#define HCSP2_MAX_SC_BUF(x) (((((x) >> 21) & 0x1f) << 5) | (((x) >> 27) & 0x1f))

// HCSPARAM3
#define HCSP3_U1_DEVICE_LATENCY(x) (((x) >>  0) & 0xff)
#define HCSP3_U2_DEVICE_LATENCY(x) (((x) >> 16) & 0xff)

// HCCPARAMS
#define HCCP_AC64(x)       (((x) >>  0) & 0x001)
#define HCCP_BNC(x)        (((x) >>  1) & 0x001)
#define HCCP_CSZ(x)        (((x) >>  2) & 0x001)
#define HCCP_PPC(x)        (((x) >>  3) & 0x001)
#define HCCP_PIND(x)       (((x) >>  4) & 0x001)
#define HCCP_LHRC(x)       (((x) >>  5) & 0x001)
#define HCCP_LTC(x)        (((x) >>  6) & 0x001)
#define HCCP_NSS(x)        (((x) >>  7) & 0x001)
#define HCCP_PAE(x)        (((x) >>  8) & 0x001)
#define HCCP_SPC(x)        (((x) >>  9) & 0x001)
#define HCCP_SEC(x)        (((x) >> 10) & 0x001)
#define HCCP_CFC(x)        (((x) >> 11) & 0x001)
#define HCCP_MAXPSASIZE(x) (((x) >> 12) & 0x00f)
#define HCCP_XECP(x)       (((x) >> 16) & 0xfff)



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

#define XHCI_OPT_REG_USBCMD    0x00
#define XHCI_OPT_REG_USBSTS    0x04
#define XHCI_OPT_REG_PAGESIZE  0x08
#define XHCI_OPT_REG_DNCTRL    0x14
#define XHCI_OPT_REG_CRCR_LO   0x18
#define XHCI_OPT_REG_CRCR_HI   0x1c
#define XHCI_OPT_REG_DCBAPP_LO 0x30
#define XHCI_OPT_REG_DCBAPP_HI 0x34
#define XHCI_OPT_REG_CONFIG    0x38

// USBCMD
#define USBCMD_RUN    (1 <<  0) // Running
#define USBCMD_HCRST  (1 <<  1) // Host Controller Reset
#define USBCMD_INTE   (1 <<  2) // IRQ Enable
#define USBCMD_HSEE   (1 <<  3) // Host System Error En
#define USBCMD_LHCRST (1 <<  7) // Light Host Controller Reset
#define USBCMD_CSS    (1 <<  8) // Controller Save State
#define USBCMD_CRS    (1 <<  9) // Controller Restore State
#define USBCMD_EWE    (1 << 10) // Enable Wrap Event

// USBSTS
#define USBSTS_HCH  (1 <<  0)  // Host Controller Halt
#define USBSTS_HSE  (1 <<  2)  // Host System Error
#define USBSTS_EINT (1 <<  3)  // Event Interrupt
#define USBSTS_PCD  (1 <<  4)  // Port Change Detect
#define USBSTS_SSS  (1 <<  8)  // Save State Status
#define USBSTS_RSS  (1 <<  9)  // Restore State Status
#define USBSTS_SRE  (1 << 10)  // Save Restore Error
#define USBSTS_CNR  (1 << 11)  // Controller Not Ready
#define USBSTS_HCE  (1 << 12)  // Host Controller Error

// CRCR
#define CRCR_RCS (1<<0)
#define CRCR_CS  (1<<1)
#define CRCR_CA  (1<<2)
#define CRCR_CRR (1<<3)



/* XHCI Runtime Regiseters
     Offset | Mnemonic
      0000h | MFINDEX Microframe Index
0004h:0020h | RsvdZ
      0020h | IR0 Interrupter Register Set 0
      ....  | ....
      8000h | IR1023 Interrupter Register Set 1023
*/

#define XHCI_RUN_REG_MFINDEX      0x00

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
#define IMAN_INTR_EN 0x00000002

// ERSTSZ
#define ERSTSZ_SET(n) ((n) & 0xffff)

// ERDP
#define ERDP_EHB(x) (((x) >> 3) & 0x01)

#define XHCI_MAX_EVENTS      (16 * 13)
#define XHCI_MAX_COMMANDS    (16 * 1)
#define XHCI_MAX_SLOTS       255
#define XHCI_MAX_PORTS       127
#define XHCI_MAX_ENDPOINTS   32
#define XHCI_MAX_SCRATCHPADS 256
#define XHCI_MAX_DEVICES     128
#define XHCI_MAX_TRANSFERS   8

#pragma pack(1)

// Section 6.2.2
typedef struct
{
    uint32_t slot0;
    uint32_t slot1;
    uint32_t slot2;
    uint32_t slot3;
    uint32_t RsvdO[4];
} xhci_slot_ctx_t;

// Section 6.2.3
typedef struct
{
    uint32_t endpoint0;
    uint32_t endpoint1;
    uint64_t endpoint2;
    uint32_t endpoint4;
    uint32_t RsvdO[3];
} xhci_endpoint_ctx_t;

// Section 6.2.1
typedef struct
{
    xhci_slot_ctx_t slot;
    xhci_endpoint_ctx_t endpoint[XHCI_MAX_ENDPOINTS - 1];
} xhci_device_ctx_t;

// Section 6.4
typedef struct
{
    uint64_t addr;
    uint32_t status;
    uint32_t flags;
} xhci_trb_t;

// Section 6.5
typedef struct
{
    uint64_t rs_addr;
    uint32_t rs_size;
    uint32_t rsvdz;
} xhci_erst_t;

#pragma pack()

typedef struct
{
    uint64_t base_addr[XHCI_MAX_SLOTS];
    struct
    {
        uint64_t padding;
    } __attribute__((__aligned__(64)));
    uint64_t scratchpad[XHCI_MAX_SCRATCHPADS];
} xhci_device_cxt_arr_t;

typedef struct
{
    uint8_t               *mmio_base;
    uint8_t               *cap_regs;
    uint8_t               *opt_regs;
    uint8_t               *run_regs;
    uint32_t               max_ports;
    uint32_t               max_slots;
    uint32_t               cxt_size;
    uint32_t               msi_vector;
    uint32_t               scrath_chapad_count;
    xhci_device_cxt_arr_t *dev_cxt_arr;
    xhci_erst_t           *erst;
    xhci_trb_t            *event_ring;
    xhci_trb_t            *command_ring;
} xhci_t;

PUBLIC void xhci_init();
PUBLIC uint32_t xhci_read_cap_reg32(uint32_t reg);
PUBLIC uint32_t xhci_read_opt_reg32(uint32_t reg);
PUBLIC void xhci_write_opt_reg32(uint32_t reg,uint32_t value);
PUBLIC uint32_t xhci_read_run_reg32(uint32_t reg);
PUBLIC void xhci_write_run_reg32(uint32_t reg,uint32_t val);

#endif