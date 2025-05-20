/*
   Copyright 2024 LinChenjun

   本程序是自由软件
   修改和/或再分发依照 GNU GPL version 3 (or any later version)

*/

#include <kernel/global.h>
#include <device/usb/xhci.h>      // xhci_t,xhci registers
#include <device/usb/xhci_trb.h>  // xhci_trb
#include <device/usb/xhci_regs.h> // xhci registers
#include <device/usb/usb.h>       // trb_type_str
#include <kernel/syscall.h>       // sys_send_recv
#include <device/pci.h>           // pci_device_t,pci functions
#include <task/task.h>            // task_yield
#include <device/cpu.h>           // is_virtual_machine

#include <log.h>

extern xhci_t *xhci_set;

// PRIVATE status_t xhci_configure_port(xhci_t *xhci,uint8_t port_id)
// {
//     status_t status = xhci_hub_port_reset(xhci,port_id);
//     if (ERROR(status))
//     {
//         pr_log("\3 Port Reset Failed.\n");
//         return status;
//     }
//     // TODO: configure slot
//     status = xhci_enable_slot(xhci);
//     return K_SUCCESS;
// }

// PRIVATE status_t port_change_event(xhci_t *xhci,xhci_trb_t *trb)
// {
//     uint8_t port_id = (trb->addr >> 24) & 0xff;
//     pr_log("\2 Port Status Change: port id: %d.",port_id);
//     uint32_t portsc;
//     portsc = xhci_read_opt(xhci,XHCI_OPT_PORTSC(port_id - 1));

//     // Clear PSC bit.
//     uint32_t portsc_clear;
//     portsc_clear = ((portsc & ~(PORTSC_PED | PORTSC_PR))
//                     & ~(PORTSC_PLS_MASK << PORTSC_PLS_SHIFT))
//                     | 1 << PORTSC_PLS_SHIFT;
//     xhci_write_opt(xhci,XHCI_OPT_PORTSC(port_id - 1),portsc_clear);

//     uint8_t is_connected = GET_FIELD(portsc,PORTSC_CCS);

//     pr_log("%s",is_connected ? "attach" : "detach");
//     pr_log("\n");
//     if (is_connected)
//     {
//         status_t status = xhci_configure_port(xhci,port_id);
//         if (ERROR(status))
//         {
//             return status;
//         }
//     }
//     return K_SUCCESS;
// }

PUBLIC const char* trb_type_str(uint8_t trb_type)
{
    switch(trb_type)
    {
        case TRB_TYPE_NORMAL: return "TRB_TYPE_NORMAL";
        case TRB_TYPE_SETUP_STAGE: return "TRB_TYPE_SETUP_STAGE";
        case TRB_TYPE_DATA_STAGE: return "TRB_TYPE_DATA_STAGE";
        case TRB_TYPE_STATUS_STAGE: return "TRB_TYPE_STATUS_STAGE";
        case TRB_TYPE_ISOCH: return "TRB_TYPE_ISOCH";
        case TRB_TYPE_LINK: return "TRB_TYPE_LINK";
        case TRB_TYPE_EVENT_DATA: return "TRB_TYPE_EVENT_DATA";
        case TRB_TYPE_TR_NOOP: return "TRB_TYPE_TR_NOOP";
        // Command
        case TRB_TYPE_ENABLE_SLOT: return "TRB_TYPE_ENABLE_SLOT";
        case TRB_TYPE_DISABLE_SLOT: return "TRB_TYPE_DISABLE_SLOT";
        case TRB_TYPE_ADDRESS_DEVICE: return "TRB_TYPE_ADDRESS_DEVICE";
        case TRB_TYPE_CONFIGURE_ENDPOINT: return "TRB_TYPE_CONFIGURE_ENDPOINT";
        case TRB_TYPE_EVALUATE_CONTEXT: return "TRB_TYPE_EVALUATE_CONTEXT";
        case TRB_TYPE_RESET_ENDPOINT: return "TRB_TYPE_RESET_ENDPOINT";
        case TRB_TYPE_STOP_ENDPOINT: return "TRB_TYPE_STOP_ENDPOINT";
        case TRB_TYPE_SET_TR_DEQUEUE: return "TRB_TYPE_SET_TR_DEQUEUE";
        case TRB_TYPE_RESET_DEVICE: return "TRB_TYPE_RESET_DEVICE";
        case TRB_TYPE_FORCE_EVENT: return "TRB_TYPE_FORCE_EVENT";
        case TRB_TYPE_NEGOCIATE_BW: return "TRB_TYPE_NEGOCIATE_BW";
        case TRB_TYPE_SET_LATENCY_TOLERANCE: return "TRB_TYPE_SET_LATENCY_TOLERANCE";
        case TRB_TYPE_GET_PORT_BW: return "TRB_TYPE_GET_PORT_BW";
        case TRB_TYPE_FORCE_HEADER: return "TRB_TYPE_FORCE_HEADER";
        case TRB_TYPE_CMD_NOOP: return "TRB_TYPE_CMD_NOOP";
        // Events
        case TRB_TYPE_TRANSFER: return "TRB_TYPE_TRANSFER";
        case TRB_TYPE_COMMAND_COMPLETION: return "TRB_TYPE_COMMAND_COMPLETION";
        case TRB_TYPE_PORT_STATUS_CHANGE: return "TRB_TYPE_PORT_STATUS_CHANGE";
        case TRB_TYPE_BANDWIDTH_REQUEST: return "TRB_TYPE_BANDWIDTH_REQUEST";
        case TRB_TYPE_DOORBELL: return "TRB_TYPE_DOORBELL";
        case TRB_TYPE_HOST_CONTROLLER: return "TRB_TYPE_HOST_CONTROLLER";
        case TRB_TYPE_DEVICE_NOTIFICATION: return "TRB_TYPE_DEVICE_NOTIFICATION";
        case TRB_TYPE_MFINDEX_WRAP: return "TRB_TYPE_MFINDEX_WRAP";
    }
    return "Unknow TRB type";
}

PRIVATE void process_intr(xhci_t *xhci)
{
    uint32_t usbsts = xhci_read_opt(xhci,XHCI_OPT_USBSTS);
    if (usbsts & (USBSTS_HCH | USBSTS_HCE | USBSTS_HSE))
    {
        if (usbsts & USBSTS_HCH)
        {
            pr_log("\3 xHCI Host Controller Halted.\n");
        }
        if (usbsts & USBSTS_HSE)
        {
            pr_log("\3 xHCI Host System Error.\n");
        }
        if (usbsts & USBSTS_HCE)
        {
            pr_log("\3 xHCI Host Controller Error.\n");
        }
    }
}

PUBLIC void process_event(xhci_t *xhci)
{
    uint16_t dequeue_index = xhci->event_ring.ring.dequeue;
    uint8_t  cycle_bit     = xhci->event_ring.ring.cycle_bit;

    xhci_trb_t *trb;
    uint8_t event_type;
    while(1)
    {
        trb         = &xhci->event_ring.ring.trbs[dequeue_index];
        event_type  = GET_FIELD(trb->flags,TRB_3_TYPE);
        if (cycle_bit != (trb->flags & TRB_3_CYCLE_BIT))
        {
            break;
        }
        switch (event_type)
        {
            case TRB_TYPE_PORT_STATUS_CHANGE:
                uint8_t  port_id = (trb->addr >> 24) & 0xff;
                uint32_t portsc;
                portsc = xhci_read_opt(xhci,XHCI_OPT_PORTSC(port_id - 1));
                if (GET_FIELD(portsc,PORTSC_CSC))
                {
                    xhci_port_connection_event_t pc_event;
                    pc_event.port_id = port_id;
                    pc_event.is_connected = GET_FIELD(portsc,PORTSC_CCS) == 1;
                    fifo_write(&xhci->port_connection_events,&pc_event);
                }
                fifo_write(&xhci->port_status_change_events,trb);
                break;
            case TRB_TYPE_COMMAND_COMPLETION:
                fifo_write(&xhci->command_completion_events,trb);
                break;
            case TRB_TYPE_TRANSFER:
                fifo_write(&xhci->transfer_completion_events,trb);
                break;
            default:
                pr_log("\2 Event type: %s.\n",trb_type_str(event_type));
                break;
        }
        dequeue_index++;
        if (dequeue_index == xhci->event_ring.ring.trb_count)
        {
            dequeue_index = 0;
            cycle_bit = !cycle_bit;
        }
        xhci->event_ring.ring.dequeue   = dequeue_index;
        xhci->event_ring.ring.cycle_bit = cycle_bit;

        uint64_t addr = (uint64_t)&xhci->event_ring.erst->rs_addr[dequeue_index];

        uint32_t erdp_lo = (addr & 0xffffffff) | (1 << 3);// clear EHB
        uint32_t erdp_hi = addr >> 32;
        xhci_write_run(xhci,XHCI_IRS_ERDP_LO(0),erdp_lo);
        xhci_write_run(xhci,XHCI_IRS_ERDP_HI(0),erdp_hi);
    }
    return;
}

PUBLIC void usb_event_task(void)
{
    uint32_t number_of_xhci = pci_dev_count(0x0c,0x03,0x30);
    uint32_t i;
    uint8_t  port;
    xhci_t *xhci;
    if (is_virtual_machine())
    {
        for (i = 0;i < number_of_xhci;i++)
        {
            xhci = &xhci_set[i];
            for (port = 0;port < xhci_set[i].max_ports;port++)
            {
                uint32_t portsc = xhci_read_opt(xhci,XHCI_OPT_PORTSC(port));
                if (portsc & PORTSC_CCS && portsc & PORTSC_CSC)
                {
                    xhci_port_connection_event_t pc_event;
                    pc_event.port_id = port + 1;
                    pc_event.is_connected = GET_FIELD(portsc,PORTSC_CCS) == 1;
                    fifo_write(&xhci->port_connection_events,&pc_event);
                }
            }
        }
    }

    while(1)
    {
        for (i = 0;i < number_of_xhci;i++)
        {
            process_intr(&xhci_set[i]);
            process_event(&xhci_set[i]);
        }
        task_yield();
    };
}