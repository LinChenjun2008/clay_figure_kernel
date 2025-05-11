/*
   Copyright 2024-2025 LinChenjun

   本程序是自由软件
   修改和/或再分发依照 GNU GPL version 3 (or any later version)

*/

#ifndef __XHCI_TRB_H__
#define __XHCI_TRB_H__

// TRB
#define TRB_3_BSR_SHIFT 9
#define TRB_3_BSR_MASK  0x01

#define TRB_3_TYPE_SHIFT 10
#define TRB_3_TYPE_MASK  0x3f

#define TRB_3_SLOT_ID_SHIFT 24
#define TRB_3_SLOT_ID_MASK  0xff

// TRB type
#define TRB_TYPE_NORMAL                 1
#define TRB_TYPE_SETUP_STAGE            2
#define TRB_TYPE_DATA_STAGE             3
#define TRB_TYPE_STATUS_STAGE           4
#define TRB_TYPE_ISOCH                  5
#define TRB_TYPE_LINK                   6
#define TRB_TYPE_EVENT_DATA             7
#define TRB_TYPE_TR_NOOP                8
// Command
#define TRB_TYPE_ENABLE_SLOT            9
#define TRB_TYPE_DISABLE_SLOT          10
#define TRB_TYPE_ADDRESS_DEVICE        11
#define TRB_TYPE_CONFIGURE_ENDPOINT    12
#define TRB_TYPE_EVALUATE_CONTEXT      13
#define TRB_TYPE_RESET_ENDPOINT        14
#define TRB_TYPE_STOP_ENDPOINT         15
#define TRB_TYPE_SET_TR_DEQUEUE        16
#define TRB_TYPE_RESET_DEVICE          17
#define TRB_TYPE_FORCE_EVENT           18
#define TRB_TYPE_NEGOCIATE_BW          19
#define TRB_TYPE_SET_LATENCY_TOLERANCE 20
#define TRB_TYPE_GET_PORT_BW           21
#define TRB_TYPE_FORCE_HEADER          22
#define TRB_TYPE_CMD_NOOP              23

// Events
#define TRB_TYPE_TRANSFER              32
#define TRB_TYPE_COMMAND_COMPLETION    33
#define TRB_TYPE_PORT_STATUS_CHANGE    34
#define TRB_TYPE_BANDWIDTH_REQUEST     35
#define TRB_TYPE_DOORBELL              36
#define TRB_TYPE_HOST_CONTROLLER       37
#define TRB_TYPE_DEVICE_NOTIFICATION   38
#define TRB_TYPE_MFINDEX_WRAP          39

#define TRB_2_COMP_CODE_SHIFT 24
#define TRB_2_COMP_CODE_MASK  0xff

#define COMP_INVALID                 0
#define COMP_SUCCESS                 1
#define COMP_DATA_BUFFER             2
#define COMP_BABBLE                  3
#define COMP_USB_TRANSACTION         4
#define COMP_TRB                     5
#define COMP_STALL                   6
#define COMP_RESOURCE                7
#define COMP_BANDWIDTH               8
#define COMP_NO_SLOTS                9
#define COMP_INVALID_STREAM         10
#define COMP_SLOT_NOT_ENABLED       11
#define COMP_ENDPOINT_NOT_ENABLED   12
#define COMP_SHORT_PACKET           13
#define COMP_RING_UNDERRUN          14
#define COMP_RING_OVERRUN           15
#define COMP_VF_RING_FULL           16
#define COMP_PARAMETER              17
#define COMP_BANDWIDTH_OVERRUN      18
#define COMP_CONTEXT_STATE          19
#define COMP_NO_PING_RESPONSE       20
#define COMP_EVENT_RING_FULL        21
#define COMP_INCOMPATIBLE_DEVICE    22
#define COMP_MISSED_SERVICE         23
#define COMP_COMMAND_RING_STOPPED   24
#define COMP_COMMAND_ABORTED        25
#define COMP_STOPPED                26
#define COMP_STOPPED_LENGTH_INVALID 27
#define COMP_MAX_EXIT_LATENCY       29
#define COMP_ISOC_OVERRUN           31
#define COMP_EVENT_LOST             32
#define COMP_UNDEFINED              33
#define COMP_INVALID_STREAM_ID      34
#define COMP_SECONDARY_BANDWIDTH    35
#define COMP_SPLIT_TRANSACTION      36

#define TRB_3_CYCLE_BIT            (1U <<  0)
#define TRB_3_TC_BIT               (1U <<  1)
#define TRB_3_ENT_BIT              (1U <<  1)
#define TRB_3_ISP_BIT              (1U <<  2)
#define TRB_3_EVENT_DATA_BIT       (1U <<  2)
#define TRB_3_NSNOOP_BIT           (1U <<  3)
#define TRB_3_CHAIN_BIT            (1U <<  4)
#define TRB_3_IOC_BIT              (1U <<  5)
#define TRB_3_IDT_BIT              (1U <<  6)
#define TRB_3_BEI_BIT              (1U <<  9)
#define TRB_3_DCEP_BIT             (1U <<  9)
#define TRB_3_PRSV_BIT             (1U <<  9)
#define TRB_3_BSR_BIT              (1U <<  9)
#define TRB_3_TRT_MASK             (3U << 16)
#define TRB_3_DIR_IN               (1U << 16)
#define TRB_3_TRT_OUT              (2U << 16)
#define TRB_3_TRT_IN               (3U << 16)
#define TRB_3_SUSPEND_ENDPOINT_BIT (1U << 23)
#define TRB_3_ISO_SIA_BIT          (1U << 31)

#define TRB_3_SLOT_SHIFT 24
#define TRB_3_SLOT_MASK  0xff

// Section 6.4
typedef struct xhci_trb_s
{
    uint64_t addr;
    uint32_t status;
    uint32_t flags;
} xhci_trb_t;

#endif