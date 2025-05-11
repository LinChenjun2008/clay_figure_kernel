/*
   Copyright 2024-2025 LinChenjun

   本程序是自由软件
   修改和/或再分发依照 GNU GPL version 3 (or any later version)

*/
#ifndef __XHCI_DEVICE_CTX_H__
#define __XHCI_DEVICE_CTX_H__

#pragma pack(1)

#define XHCI_SLOT_STATE_DISABLED_ENABLED    0
#define XHCI_SLOT_STATE_DEFAULT             1
#define XHCI_SLOT_STATE_ADDRESSED           2
#define XHCI_SLOT_STATE_CONFIGURED          3
#define XHCI_SLOT_STATE_RESERVED            4

#define SLOT_CTX_0_ROUTE_STRING_SHIFT 0
#define SLOT_CTX_0_ROUTE_STRING_MASK  0xfffff

#define SLOT_CTX_0_SPEED_SHIFT 20
#define SLOT_CTX_0_SPEED_MASK  0x0f

#define SLOT_CTX_0_CTX_ENTRY_SHIFT 27
#define SLOT_CTX_0_CTX_ENTRY_MASK  0x1f

#define SLOT_CTX_1_ROOT_HUB_PORT_NUM_SHIFT 16
#define SLOT_CTX_1_ROOT_HUB_PORT_NUM_MASK  0xff

#define SLOT_CTX_2_INTR_TARGET_SHIFT 22
#define SLOT_CTX_2_INTR_TARGET_MASK  0x3ff

#define SLOT_CTX_3_SLOT_STATE_SHIFT 27
#define SLOT_CTX_3_SLOT_STATE_MASK  0x1f

#define SLOT_CTX_3_DEVICE_ADDRESS_SHIFT 0
#define SLOT_CTX_3_DEVICE_ADDRESS_MASK  0xff

// Section 6.2.2
typedef struct xhci_slot_ctx32_s
{
    uint32_t slot0;
    uint32_t slot1;
    uint32_t slot2;
    uint32_t slot3;
    uint32_t RsvdO[4];
} xhci_slot_ctx32_t;

STATIC_ASSERT(sizeof(xhci_slot_ctx32_t) == 32,"32-btyes slot context should be 32 bytes");

#define XHCI_EP_STATE_DISABLED    0
#define XHCI_EP_STATE_RUNNING     1
#define XHCI_EP_STATE_HALTED      2
#define XHCI_EP_STATE_STOPPED     3
#define XHCI_EP_STATE_ERROR       4

#define XHCI_EP_TYPE_INVALID          0
#define XHCI_EP_TYPE_ISOCHRONOUS_OUT  1
#define XHCI_EP_TYPE_BULK_OUT         2
#define XHCI_EP_TYPE_INTERRUPT_OUT    3
#define XHCI_EP_TYPE_CONTROL          4
#define XHCI_EP_TYPE_ISOCHRONOUS_IN   5
#define XHCI_EP_TYPE_BULK_IN          6
#define XHCI_EP_TYPE_INTERRUPT_IN     7

#define EP_CTX_0_EP_STATE_SHIFT 0
#define EP_CTX_0_EP_STATE_MASK  0x07

#define EP_CTX_0_INTERVAL_SHIFT 16
#define EP_CTX_0_INTERVAL_MASK  0xff

#define EP_CTX_0_MAX_ESIT_PAYLOAD_HI_SHIFT 24
#define EP_CTX_0_MAX_ESIT_PAYLOAD_HI_MASK  0xff

#define EP_CTX_1_EP_TYPE_SHIFT 3
#define EP_CTX_1_EP_TYPE_MASK  0x07

#define EP_CTX_1_CERR_SHIFT 1
#define EP_CTX_1_CERR_MASK  0x03

#define EP_CTX_1_MAX_PACKET_SZ_SHIFT 16
#define EP_CTX_1_MAX_PACKET_SZ_MASK  0xffff

#define EP_CTX_2_DCS_SHIFT 0
#define EP_CTX_2_DCS_MASK  0x01 /* bit 0: DCS bit 1-3: RsvdZ*/

#define EP_CTX_2_TR_DEQUEUE_LO_SHIFT 0
#define EP_CTX_2_TR_DEQUEUE_LO_MASK  0xffffffff /* bit 0: DCS bit 1-3: RsvdZ*/

#define EP_CTX_3_TR_DEQUEUE_HI_SHIFT 0
#define EP_CTX_3_TR_DEQUEUE_HI_MASK  0xffffffff

#define EP_CTX_4_AVERAGE_TRB_LEN_SHIFT 0
#define EP_CTX_4_AVERAGE_TRB_LEN_MASK  0xffff

#define EP_CTX_4_MAX_ESIT_PAYLOAD_LO_SHIFT 16
#define EP_CTX_4_MAX_ESIT_PAYLOAD_LO_MASK  0xffff

// Section 6.2.3
typedef struct xhci_endpoint_ctx32_s
{
    uint32_t endpoint0;
    uint32_t endpoint1;
    uint32_t endpoint2;
    uint32_t endpoint3;
    uint32_t endpoint4;
    uint32_t RsvdO[3];
} xhci_endpoint_ctx32_t;

STATIC_ASSERT(sizeof(xhci_endpoint_ctx32_t) == 32,"32-btyes endpoint context should be 32 bytes");

typedef struct xhci_input_ctrl_ctx32_s
{
    uint32_t drop_flags;
    uint32_t add_flags;
    uint32_t rsvd[5];
    uint8_t  config_value;
    uint8_t  interface_number;
    uint8_t  alternate_setting;
    uint8_t  RsvdZ;
} xhci_input_ctrl_ctx32_t;

STATIC_ASSERT(sizeof(xhci_input_ctrl_ctx32_t) == 32,"32-byte input control context should be 32 bytes");

typedef struct xhci_device_ctx32_s
{
    xhci_slot_ctx32_t     slot_context;
    xhci_endpoint_ctx32_t control_ep_context;
    xhci_endpoint_ctx32_t ep[30];
} xhci_device_ctx32_t;

STATIC_ASSERT(sizeof(xhci_device_ctx32_t) == 1024,"32-byte Device context should be 1024 bytes");

typedef struct xhci_input_ctx32_s
{
    xhci_input_ctrl_ctx32_t        control_context;
    xhci_device_ctx32_t            device_context;
} xhci_input_ctx32_t;

STATIC_ASSERT(sizeof(xhci_input_ctx32_t) == 1056,"32-byte input context should be 1056 bytes");

// 64-byte device context

typedef struct xhci_slot_ctx64_s
{
    uint32_t slot0;
    uint32_t slot1;
    uint32_t slot2;
    uint32_t slot3;
    uint32_t RsvdO[12];
} xhci_slot_ctx64_t;

STATIC_ASSERT(sizeof(xhci_slot_ctx64_t) == 64,"64-btyes slot context should be 64 bytes");

typedef struct xhci_endpoint_ctx64_s
{
    uint32_t endpoint0;
    uint32_t endpoint1;
    uint32_t endpoint2;
    uint32_t endpoint3;
    uint32_t endpoint4;
    uint32_t RsvdO[11];
} xhci_endpoint_ctx64_t;

STATIC_ASSERT(sizeof(xhci_endpoint_ctx64_t) == 64,"64-btyes endpoint context should be 64 bytes");

typedef struct xhci_input_ctrl_ctx64_s
{
    uint32_t drop_flags;
    uint32_t add_flags;
    uint32_t rsvd[5];
    uint8_t  config_value;
    uint8_t  interface_number;
    uint8_t  alternate_setting;
    uint8_t  RsvdZ;
    uint32_t padding[8];
} xhci_input_ctrl_ctx64_t;

STATIC_ASSERT(sizeof(xhci_input_ctrl_ctx64_t) == 64,"64-byte input control context should be 64 bytes");

typedef struct xhci_device_ctx64_s
{
    xhci_slot_ctx64_t     slot_context;
    xhci_endpoint_ctx64_t control_ep_context;
    xhci_endpoint_ctx64_t ep[30];
} xhci_device_ctx64_t;

STATIC_ASSERT(sizeof(xhci_device_ctx64_t) == 2048,"64-byte Device context should be 2048 bytes");

typedef struct xhci_input_ctx64_s
{
    xhci_input_ctrl_ctx64_t        control_context;
    xhci_device_ctx64_t            device_context;
} xhci_input_ctx64_t;

STATIC_ASSERT(sizeof(xhci_input_ctx64_t) == 2112,"64-byte input context should be 2112 bytes");

#pragma pack()

PUBLIC status_t init_xhci_device_struct(
    xhci_device_t *device,
    uint8_t port_id,
    uint8_t slot_id,
    uint8_t speed,
    uint16_t using_64byte_ctx);

PUBLIC xhci_input_ctrl_ctx32_t* xhci_device_get_input_ctrl_ctx(
    xhci_device_t *device);
PUBLIC xhci_slot_ctx32_t* xhci_device_get_input_slot_ctx(xhci_device_t *device);
PUBLIC xhci_endpoint_ctx32_t* xhci_device_get_input_ctrl_ep_ctx(
    xhci_device_t *device);

#endif