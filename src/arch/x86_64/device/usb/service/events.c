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
#include <std/string.h>           // memset

#include <log.h>

extern xhci_t *xhci_set;

// PRIVATE status_t xhci_hub_port_reset(xhci_t *xhci,uint8_t port_id)
// {
//     uint32_t portsc = xhci_read_opt(xhci,XHCI_OPT_PORTSC(port_id - 1));
//     if (!GET_FIELD(portsc,PORTSC_CCS))
//     {
//         return K_ERROR;
//     }
//     switch (GET_FIELD(portsc,PORTSC_PLS))
//     {
//         case PLS_U0:
//             // Section 4.3
//             // USB 3 - controller automatically performs reset
//             break;
//         case PLS_POLLING:
//             // USB 2 - Reset Port
//             xhci_write_opt(xhci,XHCI_OPT_PORTSC(port_id - 1),portsc | PORTSC_PR);
//             break;
//         default:
//             return K_UDF_BEHAVIOR;
//     }
//     while (1)
//     {
//         portsc = xhci_read_opt(xhci,XHCI_OPT_PORTSC(port_id - 1));
//         if (!GET_FIELD(portsc,PORTSC_CCS))
//         {
//             return K_ERROR;
//         }
//         if (GET_FIELD(portsc,PORTSC_PED))
//         {
//             // Success
//             break;
//         }
//     }
//     return K_SUCCESS;
// }

// PRIVATE status_t xhci_enable_slot(xhci_t *xhci)
// {
//     xhci_trb_t trb;
//     memset(&trb,0,sizeof(trb));
//     trb.flags = TRB_3_TYPE(TRB_TYPE_ENABLE_SLOT);
//     xhci_submit_command(xhci,&trb);
//     return K_SUCCESS;
// }

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

// PRIVATE void command_completion_event(xhci_t *xhci,xhci_trb_t *trb)
// {
//     (void)xhci;
//     xhci_trb_t *rtrb = (void*)(trb->addr & 0xffffffff);
//     pr_log("\1 Command completion.rtrb: %p.\n",rtrb);
//     return;
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

PUBLIC void process_event(xhci_t *xhci)
{
    uint16_t dequeue_index = xhci->event_ring.dequeue_index;
    uint8_t  cycle_bit     = xhci->event_ring.cycle_bit;

    xhci_trb_t trb = xhci->event_ring.ring[dequeue_index];
    uint8_t event_type = GET_FIELD(trb.flags,TRB_3_TYPE);
    while(cycle_bit == (trb.flags & TRB_3_CYCLE_BIT))
    {
        trb = xhci->event_ring.ring[dequeue_index];
        event_type = GET_FIELD(trb.flags,TRB_3_TYPE);
        switch (event_type)
        {
            // case TRB_TYPE_PORT_STATUS_CHANGE:
            //     port_change_event(xhci,&xhci->event_ring.ring[dequeue_index]);
            //     break;
            // case TRB_TYPE_COMMAND_COMPLETION:
            //     command_completion_event(xhci,&xhci->event_ring.ring[dequeue_index]);
            //     break;
            default:
                pr_log("\2 Event type: %s.\n",trb_type_str(event_type));
                break;
        }
        dequeue_index++;
        if (dequeue_index == xhci->event_ring.trb_count)
        {
            dequeue_index = 0;
            cycle_bit = !cycle_bit;
        }
        xhci->event_ring.dequeue_index = dequeue_index;
        xhci->event_ring.cycle_bit     = cycle_bit;

        uint64_t addr =   xhci->event_ring.erst->rs_addr
                        + dequeue_index * sizeof(xhci_trb_t);

        uint32_t erdp_lo = (addr & 0xffffffff) | (1 << 3);// clear EHB
        uint32_t erdp_hi = addr >> 32;
        xhci_write_run(xhci,XHCI_IRS_ERDP_LO(0),erdp_lo);
        xhci_write_run(xhci,XHCI_IRS_ERDP_HI(0),erdp_hi);
    }
}