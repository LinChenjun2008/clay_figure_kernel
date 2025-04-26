/*
   Copyright 2024-2025 LinChenjun

   本程序是自由软件
   修改和/或再分发依照 GNU GPL version 3 (or any later version)

*/

#ifndef __XHCI_REGS_H__
#define __XHCI_REGS_H__


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

#define GET_HCSP2_MAX_SC_BUF(x) ((GET_FIELD(x,HCSP2_MAX_SC_BUF_HI) << 5) \
                                 | GET_FIELD(x,HCSP2_MAX_SC_BUF_LO))

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

// Section 7
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

#define XHCI_OPT_PORTSC(n)    (0x400 + ((n) * 0x10)) // n: Port Number-1 (0,1,2,...,Max Port-1)
#define XHCI_OPT_PORTPMSC(n)  (0x404 + ((n) * 0x10)) // n: Port Number-1 (0,1,2,...,Max Port-1)
#define XHCI_OPT_PORTLI(n)    (0x408 + ((n) * 0x10)) // n: Port Number-1 (0,1,2,...,Max Port-1)
#define XHCI_OPT_PORTHLPMC(n) (0x40c + ((n) * 0x10)) // n: Port Number-1 (0,1,2,...,Max Port-1)

#define PORTSC_CCS (1 <<  0) // Current Connect Status
#define PORTSC_PED (1 <<  1) // Port Enabled/Disabled
#define PORTSC_PR  (1 <<  4) // Port Reset
#define PORTSC_CSC (1 << 17) // Connect Status Change
#define PORTSC_PRC (1 << 21) // Port Reset Change

#define PORTSC_CCS_SHIFT 0
#define PORTSC_CCS_MASK  0x01

#define PORTSC_PED_SHIFT 1
#define PORTSC_PED_MASK  0x01

#define PORTSC_PR_SHIFT 4
#define PORTSC_PR_MASK  0x01

#define PORTSC_PLS_SHIFT 5
#define PORTSC_PLS_MASK  0x0f

#define PORTSC_PP_SHIFT 9
#define PORTSC_PP_MASK  0x01

#define PORTSC_SPEED_SHIFT 10
#define PORTSC_SPEED_MASK  0x0f

#define PORTSC_CSC_SHIFT 17
#define PORTSC_CSC_MASK  0x01

#define PORTSC_PRC_SHIFT 21
#define PORTSC_PRC_MASK  0x01

#define PLS_U0               0
#define PLS_U1               1
#define PLS_U2               2
#define PLS_U3               3
#define PLS_DISABLED         4
#define PLS_RX_DETECT        5
#define PLS_INACTIVE         6
#define PLS_POLLING          7
#define PLS_RECOVERY         8
#define PLS_HOT_RESET        9
#define PLS_COMPILANCE_MODE 10
#define PLS_TEST_MODE       11
#define PLS_RESUME          15

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

#define USBSTS_PCD_SHIFT 4
#define USBSTS_PCD_MASK  0x01

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

#define XHCI_RUN_MFINDEX      0x00

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

#endif