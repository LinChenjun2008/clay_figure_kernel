/*
   Copyright 2024 LinChenjun

   本程序是自由软件
   修改和/或再分发依照 GNU GPL version 3 (or any later version)

*/

#include <kernel/global.h>
#include <kernel/syscall.h>              // inform_intr,sys_send_recv
#include <service.h>                     // TICK_SLEEP
#include <device/pci.h>                  // pci_device_t,pci functions
#include <device/usb/xhci.h>             // xhci_t
#include <device/usb/xhci_regs.h>        // xhci registers
#include <device/usb/xhci_trb.h>         // xhci_trb_t
#include <device/usb/xhci_mem.h>         // xhci memory
#include <device/usb/xhci_device_ctx.h>  // xhci device context
#include <device/usb/usb_desc.h>         // usb device descriptor
#include <device/usb/usb.h>              // process_event
#include <task/task.h>                   // task_start
#include <std/string.h>                  // memset
#include <mem/mem.h>                     // pmalloc

#include <log.h>

extern xhci_t *xhci_set;

PRIVATE status_t xhci_hub_port_reset(xhci_t *xhci,uint8_t port_id)
{
    uint8_t usb3_start = xhci->usb3.start;
    uint8_t usb3_end   = xhci->usb3.start + xhci->usb3.count;
    uint8_t is_usb3_port = usb3_start <= port_id && port_id < usb3_end;

    uint32_t portsc = xhci_read_opt(xhci,XHCI_OPT_PORTSC(port_id - 1));
    // Power on the port if necessary
    if (GET_FIELD(portsc,PORTSC_PP) == 0)
    {
        portsc = SET_FIELD(portsc,PORTSC_PP,1);
        xhci_write_opt(xhci,XHCI_OPT_PORTSC(port_id - 1),portsc);

        // Check
        portsc = xhci_read_opt(xhci,XHCI_OPT_PORTSC(port_id - 1));
        if (GET_FIELD(portsc,PORTSC_PP) == 0)
        {
            pr_log("\3 Failed to power on the port %d.\n",port_id);
            return K_ERROR;
        }
    }

    // Clear lingering status change bits before reset
    // Clear bits by write
    portsc = SET_FIELD(portsc,PORTSC_CSC,1);
    portsc = SET_FIELD(portsc,PORTSC_PEC,1);
    portsc = SET_FIELD(portsc,PORTSC_PRC,1);
    xhci_write_opt(xhci,XHCI_OPT_PORTSC(port_id - 1),portsc);

    if (is_usb3_port)
    {
        portsc = SET_FIELD(portsc,PORTSC_WPR,1);
    }
    else
    {
        portsc = SET_FIELD(portsc,PORTSC_PR,1);
    }
    xhci_write_opt(xhci,XHCI_OPT_PORTSC(port_id - 1),portsc);

    while (1)
    {
        portsc = xhci_read_opt(xhci,XHCI_OPT_PORTSC(port_id - 1));
        if (is_usb3_port && (GET_FIELD(portsc,PORTSC_WRC) == 1))
        {
            break;
        }
        if (!is_usb3_port && (GET_FIELD(portsc,PORTSC_PRC) == 1))
        {
            break;
        }
    }
    portsc = SET_FIELD(portsc,PORTSC_CSC,1);
    portsc = SET_FIELD(portsc,PORTSC_PRC,1);
    portsc = SET_FIELD(portsc,PORTSC_WRC,1);
    portsc = SET_FIELD(portsc,PORTSC_PEC,1);
    portsc = SET_FIELD(portsc,PORTSC_PED,0);
    xhci_write_opt(xhci,XHCI_OPT_PORTSC(port_id - 1),portsc);

    // Check if the port is enabled
    portsc = xhci_read_opt(xhci,XHCI_OPT_PORTSC(port_id - 1));
    if (GET_FIELD(portsc,PORTSC_PED) == 0)
    {
        pr_log("\3 Error: port not enabled.\n");
        return K_ERROR;
    }
    return K_SUCCESS;
}

PRIVATE const char* usb_speed_to_string(uint8_t speed)
{
    static const char *speed_string[7] =
    {
        "Invalid",
        "Full Speed (12 MB/s - USB2.0)",
        "Low Speed (1.5 Mb/s - USB 2.0)",
        "High Speed (480 Mb/s - USB 2.0)",
        "Super Speed (5 Gb/s - USB3.0)",
        "Super Speed Plus (10 Gb/s - USB 3.1)",
        "Undefined",
    };
    return speed_string[speed];
}

// PUBLIC const char* port_link_status_str(uint8_t pls)
// {
//     switch (pls)
//     {
//         case PLS_U0: return "U0";
//         case PLS_U1: return "U1";
//         case PLS_U2: return "U2";
//         case PLS_U3: return "U3";
//         case PLS_DISABLED: return "DISABLED";
//         case PLS_RX_DETECT: return "RX_DETECT";
//         case PLS_INACTIVE: return "INACTIVE";
//         case PLS_POLLING: return "POLLING";
//         case PLS_RECOVERY: return "RECOVERY";
//         case PLS_HOT_RESET: return "HOT_RESET";
//         case PLS_COMPILANCE_MODE: return "COMPILANCE_MODE";
//         case PLS_TEST_MODE: return "TEST_MODE";
//         case PLS_RESUME: return "RESUME";
//     }
//     return "Unknow PLS";
// }

PRIVATE uint16_t xhci_get_max_init_packet_size(uint8_t speed)
{
    uint16_t initial_max_packet_size = 512;
    switch (speed)
    {
        case XHCI_USB_SPEED_LOW_SPEED:
            initial_max_packet_size = 8;
            break;

        case XHCI_USB_SPEED_FULL_SPEED:
        case XHCI_USB_SPEED_HIGH_SPEED:
            initial_max_packet_size = 64;
            break;

        case XHCI_USB_SPEED_SUPER_SPEED:
        case XHCI_USB_SPEED_SUPER_SPEED_PLUS:
            initial_max_packet_size = 512;
            break;
    }
    return initial_max_packet_size;
}

PRIVATE status_t xhci_enable_slot(xhci_t *xhci,uint8_t *slot_id)
{
    xhci_trb_t trb;
    memset(&trb,0,sizeof(trb));
    trb.flags = SET_FIELD(trb.flags,TRB_3_TYPE,TRB_TYPE_ENABLE_SLOT);
    status_t status;
    status = xhci_submit_command(xhci,&trb);

    *slot_id = GET_FIELD(trb.flags,TRB_3_SLOT_ID);
    return status;
}

PRIVATE status_t xhci_create_device_context(xhci_t *xhci,uint8_t slot_id)
{
    uint64_t dev_ctx_size = sizeof(xhci_device_ctx32_t);
    if (xhci->csz)
    {
        dev_ctx_size = sizeof(xhci_device_ctx64_t);
    }
    void *ctx;
    status_t status;
    status = pmalloc(dev_ctx_size,
                     XHCI_DEVICE_CONTEXT_ALIGNMENT,
                     XHCI_DEVICE_CONTEXT_BOUNDARY,
                     &ctx);
    if (ERROR(status))
    {
        pr_log("\3 Failed to alloc device context.\n");
        return status;
    }
    xhci->dcbaa[slot_id]       = (uint64_t)ctx;
    xhci->dcbaa_vaddr[slot_id] = (uint64_t)KADDR_P2V(ctx);
    return K_SUCCESS;
}

PRIVATE void xhci_configure_device_ctrl_ep_input_ctx(xhci_device_t *device,
                                                     uint16_t max_packet_size)
{
    xhci_input_ctrl_ctx32_t *input_ctrl_ctx =
                                    xhci_device_get_input_ctrl_ctx(device);
    xhci_slot_ctx32_t *slot_ctx             =
                                    xhci_device_get_input_slot_ctx(device);
    xhci_endpoint_ctx32_t *ctrl_ep_ctx      =
                                    xhci_device_get_input_ctrl_ep_ctx(device);

    // Enable slot and control endpoint contexts
    input_ctrl_ctx->add_flags  = (1 << 0) | (1 << 1);
    input_ctrl_ctx->drop_flags = 0;

    uint32_t slot0 = 0,slot1 = 0,slot2 = 0;
    slot0 = SET_FIELD(slot0,SLOT_CTX_0_ROUTE_STRING,0);
    slot0 = SET_FIELD(slot0,SLOT_CTX_0_CTX_ENTRY,1);
    slot0 = SET_FIELD(slot0,SLOT_CTX_0_SPEED,device->speed);
    slot1 = SET_FIELD(slot1,SLOT_CTX_1_ROOT_HUB_PORT_NUM,device->port_id);
    slot2 = SET_FIELD(slot2,SLOT_CTX_2_INTR_TARGET,0);

    slot_ctx->slot0 = slot0;
    slot_ctx->slot1 = slot1;
    slot_ctx->slot2 = slot2;

    // Configure Endpoint context
    uint32_t ep0 = 0,ep1 = 0,ep2 = 0,ep3 = 0,ep4 = 0;
    ep0 = SET_FIELD(ep0,EP_CTX_0_EP_STATE,XHCI_EP_STATE_DISABLED);
    ep0 = SET_FIELD(ep0,EP_CTX_0_INTERVAL,0);
    ep0 = SET_FIELD(ep0,EP_CTX_0_MAX_ESIT_PAYLOAD_HI,0);

    ep1 = SET_FIELD(ep1,EP_CTX_1_CERR,3);
    ep1 = SET_FIELD(ep1,EP_CTX_1_EP_TYPE,XHCI_EP_TYPE_CONTROL);
    ep1 = SET_FIELD(ep1,EP_CTX_1_MAX_PACKET_SZ,max_packet_size);

    xhci_transfer_ring_t *t_ring = device->ctrl_ep_ring;
    phy_addr_t transfer_ring =   (phy_addr_t)t_ring->ring.trbs_paddr;
    uint32_t tr_dequeue_lo   =   (transfer_ring & 0xffffffff)
                             | t_ring->ring.cycle_bit;
    uint32_t tr_dequeue_hi = transfer_ring >> 32;

    ep2 = SET_FIELD(ep2,EP_CTX_2_TR_DEQUEUE_LO,tr_dequeue_lo);
    ep3 = SET_FIELD(ep3,EP_CTX_3_TR_DEQUEUE_HI,tr_dequeue_hi);

    ep4 = SET_FIELD(ep4,EP_CTX_4_AVERAGE_TRB_LEN,8);
    ep4 = SET_FIELD(ep4,EP_CTX_4_MAX_ESIT_PAYLOAD_LO,0);
    ctrl_ep_ctx->endpoint0 = ep0;
    ctrl_ep_ctx->endpoint1 = ep1;
    ctrl_ep_ctx->endpoint2 = ep2;
    ctrl_ep_ctx->endpoint3 = ep3;
    ctrl_ep_ctx->endpoint4 = ep4;
    return;
}

PRIVATE status_t xhci_address_device(
    xhci_t *xhci,
    xhci_device_t *device,
    uint8_t bsr)
{
    xhci_trb_t trb;
    memset(&trb,0,sizeof(trb));
    uint32_t flags = 0;
    trb.addr = device->dma_input_ctx;
    flags = SET_FIELD(flags,TRB_3_BSR,bsr);
    flags = SET_FIELD(flags,TRB_3_TYPE,TRB_TYPE_ADDRESS_DEVICE);
    flags = SET_FIELD(flags,TRB_3_SLOT_ID,device->slot_id);
    trb.flags = flags;
    status_t status;
    status = xhci_submit_command(xhci,&trb);
    return status;
}

PRIVATE status_t xhci_submit_usb_request_packet(
    xhci_t *xhci,
    xhci_device_t *device,
    xhci_device_request_packet_t *req,
    void *buffer,
    uint32_t length)
{
    xhci_transfer_ring_t *transfer_ring =
        xhci_get_device_ctrl_ep_transfer_ring(device);

    status_t status;
    uint32_t *transfer_status_paddr;
    uint32_t *transfer_status_buffer;
    status = pmalloc(
        sizeof(*transfer_status_paddr),16,16,&transfer_status_paddr);

    if (ERROR(status))
    {
        pr_log("\3 Failed to alloc memory for transfer status.\n");
        return status;
    }
    transfer_status_buffer = KADDR_P2V(transfer_status_paddr);
    *transfer_status_buffer = 0;

    uint8_t *desc_paddr;
    uint8_t *desc_buffer;
    status = pmalloc(256,256,256,&desc_paddr);
    if (ERROR(status))
    {
        pr_log("\3 Failed to alloc memory for desc buffer.\n");
        return status;
    }
    desc_buffer = KADDR_P2V(desc_paddr);
    memset(desc_buffer,0,256);

    uint32_t trb2 = 0,trb3 = 0;

    // Setup Stage
    xhci_trb_t setup_stage_trb;
    memset(&setup_stage_trb,0,sizeof(setup_stage_trb));
    trb2 = SET_FIELD(trb2,TRB_2_TRANSFER_LEN,8);
    trb3 = SET_FIELD(trb3,SETUP_TRB_3_IDT,1);
    trb3 = SET_FIELD(trb3,TRB_3_TYPE,TRB_TYPE_SETUP_STAGE);
    trb3 = SET_FIELD(trb3,SETUP_TRB_3_TRT,3);

    setup_stage_trb.addr   = *(uint64_t*)req;
    setup_stage_trb.status = trb2;
    setup_stage_trb.flags  = trb3;

    // Data Stage
    trb2 = 0,trb3 = 0;
    xhci_trb_t data_stage_trb;
    memset(&data_stage_trb,0,sizeof(data_stage_trb));
    trb2 = SET_FIELD(trb2,TRB_2_TRANSFER_LEN,length);
    trb3 = SET_FIELD(trb3,TRB_3_TYPE,TRB_TYPE_DATA_STAGE);
    trb3 = SET_FIELD(trb3,DATA_TRB_3_CHAIN,1);
    trb3 = SET_FIELD(trb3,DATA_TRB_3_DIR,1);

    data_stage_trb.addr   = (uint64_t)desc_paddr;
    data_stage_trb.status = trb2;
    data_stage_trb.flags  = trb3;

    // Event Data First
    trb2 = 0,trb3 = 0;
    xhci_trb_t event_data_first_trb;
    memset(&event_data_first_trb,0,sizeof(event_data_first_trb));
    trb3 = SET_FIELD(trb3,TRB_3_TYPE,TRB_TYPE_EVENT_DATA);
    trb3 = SET_FIELD(trb3,EVT_DATA_TRB_3_IOC,1);

    event_data_first_trb.addr   = (uint64_t)transfer_status_paddr;
    event_data_first_trb.status = trb2;
    event_data_first_trb.flags  = trb3;

    xhci_trb_enqueue(&transfer_ring->ring,&setup_stage_trb);
    xhci_trb_enqueue(&transfer_ring->ring,&data_stage_trb);
    xhci_trb_enqueue(&transfer_ring->ring,&event_data_first_trb);

    trb2 = 0,trb3 = 0;
    xhci_trb_t status_stage_trb;
    memset(&status_stage_trb,0,sizeof(status_stage_trb));
    trb3 = SET_FIELD(trb3,TRB_3_TYPE,TRB_TYPE_STATUS_STAGE);
    trb3 = SET_FIELD(trb3,TRB_3_CHAIN_BIT,1);
    status_stage_trb.addr   = 0;
    status_stage_trb.status = trb2;
    status_stage_trb.flags  = trb3;


    // Event Data Second
    trb2 = 0,trb3 = 0;
    xhci_trb_t event_data_second_trb;
    memset(&event_data_second_trb,0,sizeof(event_data_second_trb));
    trb3 = SET_FIELD(trb3,TRB_3_TYPE,TRB_TYPE_EVENT_DATA);
    trb3 = SET_FIELD(trb3,EVT_DATA_TRB_3_IOC,1);

    event_data_second_trb.addr   = (uint64_t)transfer_status_paddr;
    event_data_second_trb.status = trb2;
    event_data_second_trb.flags  = trb3;

    xhci_trb_enqueue(&transfer_ring->ring,&status_stage_trb);
    xhci_trb_enqueue(&transfer_ring->ring,&event_data_second_trb);

    xhci_trb_t comp_trb;
    xhci_start_ctrl_ep_transfer(xhci,transfer_ring,&comp_trb);

    // Copy the descriptor
    memcpy(buffer,desc_buffer,length);

    pfree(transfer_status_paddr);
    pfree(desc_paddr);
    return K_SUCCESS;
}

PRIVATE status_t xhci_get_device_desc(
    xhci_t *xhci,
    xhci_device_t *device,
    usb_device_desc_t *usb_desc,
    uint32_t length)
{
    xhci_device_request_packet_t req;
    req.bRequestType = 0x80;
    req.bRequest     = 6;
    req.wValue       = USB_DESCRIPTOR_REQUEST(USB_DESCRIPTOR_DEVICE,0);
    req.wIndex       = 0;
    req.wLegnth      = length;
    return xhci_submit_usb_request_packet(xhci,device,&req,usb_desc,length);
}

PRIVATE status_t xhci_setup_device(xhci_t *xhci,uint8_t port_id)
{
    pr_log("\2 Port ID: %d.\n",port_id);
    uint8_t port = port_id - 1;

    uint32_t portsc;
    portsc = xhci_read_opt(xhci,XHCI_OPT_PORTSC(port));

    uint8_t speed = GET_FIELD(portsc,PORTSC_SPEED);
    uint16_t max_packet_size = xhci_get_max_init_packet_size(speed);

    status_t status;
    uint8_t slot_id;
    status = xhci_enable_slot(xhci,&slot_id);
    if (ERROR(status))
    {
        pr_log("\3 Failed to enable device slot.\n");
        return status;
    }
    pr_log("\2 Slot ID: %d.\n",slot_id);
    if (slot_id == 0)
    {
        pr_log("\3 Invalid slot id.\n");
        return K_INVAILD_PARAM;
    }
    xhci_create_device_context(xhci,slot_id);

    xhci_device_t *device;
    status = pmalloc(sizeof(*device),0,0,&device);
    if (ERROR(status))
    {
        pr_log("\3 Failed to alloc memory for device.\n");
        return status;
    }
    device = KADDR_P2V(device);
    memset(device,0,sizeof(*device));
    init_xhci_device_struct(device,port_id,slot_id,speed,xhci->csz);
    xhci_configure_device_ctrl_ep_input_ctx(device,max_packet_size);

    status = xhci_address_device(xhci,device,1);
    if (ERROR(status))
    {
        pr_log("\3 Failed to address device.\n");
        return status;
    }
    pr_log("\1 Address device successful.\n");
    usb_device_desc_t *usb_desc;
    status = pmalloc(sizeof(*usb_desc),0,0,&usb_desc);
    if (ERROR(status))
    {
        pr_log("\3 Failed to alloc usb desc.\n");
        return status;
    }
    usb_desc = KADDR_P2V(usb_desc);
    status = xhci_get_device_desc(xhci,device,usb_desc,8);
    return K_SUCCESS;
}

PRIVATE void process_connection_event(xhci_t *xhci)
{
    status_t status;
    xhci_port_connection_event_t event;
    fifo_read(&xhci->port_connection_events,&event);
    uint8_t port_id      = event.port_id;
    uint8_t is_connected = event.is_connected;

    uint32_t portsc;
    portsc = xhci_read_opt(xhci,XHCI_OPT_PORTSC(port_id - 1));
    if (is_connected)
    {
        status = xhci_hub_port_reset(xhci,port_id);
        if (ERROR(status))
        {
            pr_log("\3 Failed to reset port. ID = %d.\n",port_id);
        }
        uint8_t speed = GET_FIELD(portsc,PORTSC_SPEED);
        pr_log("\1 Port Reset Successful: %s\n",usb_speed_to_string(speed));
        xhci_setup_device(xhci,port_id);
    }
    else
    {
        pr_log("\1 Port disconnected: %d.\n",port_id);
        xhci_hub_port_reset(xhci,port_id);
    }
    return;
}

PUBLIC void usb_main(void)
{
    pr_log("\1 USB Service Start.\n");
    xhci_setup();
    xhci_start();

    task_start("USB Event",DEFAULT_PRIORITY,4096,usb_event_task,0);

    // message_t msg;
    while (1)
    {
        // sys_send_recv(NR_RECV,RECV_FROM_ANY,&msg);

        uint32_t number_of_xhci = pci_dev_count(0x0c,0x03,0x30);
        uint32_t i;

        for (i = 0;i < number_of_xhci;i++)
        {
            xhci_t *xhci = &xhci_set[i];
            if (fifo_empty(&xhci->port_connection_events))
            {
                continue;
            }
            process_connection_event(xhci);
        }
        task_yield();
    }
}