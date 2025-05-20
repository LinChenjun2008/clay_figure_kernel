/*
   Copyright 2024-2025 LinChenjun

   本程序是自由软件
   修改和/或再分发依照 GNU GPL version 3 (or any later version)

*/

#ifndef __XHCI_TRB_H__
#define __XHCI_TRB_H__

// TRB

// Transfer length,always 8.
#define TRB_2_TRANSFER_LEN_SHIFT 0
#define TRB_2_TRANSFER_LEN_MASK  0x1ffff

#define TRB_2_COMP_CODE_SHIFT    24
#define TRB_2_COMP_CODE_MASK     0xff

#define TRB_3_CHAIN_BIT_SHIFT    4
#define TRB_3_CHAIN_BIT_MASK     0x01

#define TRB_3_BSR_SHIFT          9
#define TRB_3_BSR_MASK           0x01

#define TRB_3_TYPE_SHIFT         10
#define TRB_3_TYPE_MASK          0x3f

#define TRB_3_SLOT_ID_SHIFT      24
#define TRB_3_SLOT_ID_MASK       0xff

#define SETUP_TRB_3_IDT_SHIFT    6
#define SETUP_TRB_3_IDT_MASK     0x01

#define SETUP_TRB_3_TRT_SHIFT    16
#define SETUP_TRB_3_TRT_MASK     0x03

#define DATA_TRB_3_CHAIN_SHIFT   4
#define DATA_TRB_3_CHAIN_MASK    0x01

#define DATA_TRB_3_DIR_SHIFT     16
#define DATA_TRB_3_DIR_MASK      0x01

#define EVT_DATA_TRB_3_IOC_SHIFT 5
#define EVT_DATA_TRB_3_IOC_MASK  0x01

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

// Code Complete Code
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

#pragma pack(1)

// Section 6.4
typedef struct xhci_trb_s
{
    uint64_t addr;
    uint32_t status;
    uint32_t flags;
} xhci_trb_t;

// USB Standard Descriptor Types
#define USB_DESCRIPTOR_DEVICE                          0x01
#define USB_DESCRIPTOR_CONFIGURATION                   0x02
#define USB_DESCRIPTOR_STRING                          0x03
#define USB_DESCRIPTOR_INTERFACE                       0x04
#define USB_DESCRIPTOR_ENDPOINT                        0x05
#define USB_DESCRIPTOR_DEVICE_QUALIFIER                0x06
#define USB_DESCRIPTOR_OTHER_SPEED_CONFIGURATION       0x07
#define USB_DESCRIPTOR_INTERFACE_POWER                 0x08
#define USB_DESCRIPTOR_OTG                             0x09
#define USB_DESCRIPTOR_DEBUG                           0x0A
#define USB_DESCRIPTOR_INTERFACE_ASSOCIATION           0x0B
#define USB_DESCRIPTOR_BOS                             0x0F
#define USB_DESCRIPTOR_DEVICE_CAPABILITY               0x10
#define USB_DESCRIPTOR_WIRELESS_ENDPOINT_COMPANION     0x11
#define USB_DESCRIPTOR_SUPERSPEED_ENDPOINT_COMPANION   0x30
#define USB_DESCRIPTOR_SUPERSPEEDPLUS_ISO_ENDPOINT_COMPANION 0x31

// HID Class-Specific Descriptor Types
#define USB_DESCRIPTOR_HID                             0x21
#define USB_DESCRIPTOR_HID_REPORT                      0x22
#define USB_DESCRIPTOR_HID_PHYSICAL_REPORT             0x23

// Hub Descriptor Types
#define USB_DESCRIPTOR_HUB                             0x29
#define USB_DESCRIPTOR_SUPERSPEED_HUB                  0x2A

// Billboarding Descriptor Type
#define USB_DESCRIPTOR_BILLBOARD                       0x0D

// Type-C Bridge Descriptor Type
#define USB_DESCRIPTOR_TYPE_C_BRIDGE                   0x0E

#define USB_DESCRIPTOR_REQUEST(type, index) ((type << 8) | index)

typedef struct xhci_device_request_packet_s
{
    uint8_t bRequestType;
    uint8_t bRequest;
    uint16_t wValue;
    uint16_t wIndex;
    uint16_t wLegnth;
} xhci_device_request_packet_t;

STATIC_ASSERT(sizeof(xhci_device_request_packet_t) == 8,"");

#pragma pack()

#endif