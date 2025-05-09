/*
   Copyright 2024 LinChenjun

   本程序是自由软件
   修改和/或再分发依照 GNU GPL version 3 (or any later version)

*/

#include <kernel/global.h>
#include <device/usb/xhci.h> // xhci_t,xhci registers
#include <device/usb/usb.h>  // trb_type_str
#include <std/string.h>      // memset

#include <log.h>

extern xhci_t *xhci_set;

PRIVATE status_t xhci_hub_port_reset(xhci_t *xhci,uint8_t port_id)
{
    uint32_t portsc = xhci_read_opt(xhci,XHCI_OPT_PORTSC(port_id - 1));
    if (!GET_FIELD(portsc,PORTSC_CCS))
    {
        return K_ERROR;
    }
    switch (GET_FIELD(portsc,PORTSC_PLS))
    {
        case PLS_U0:
            // Section 4.3
            // USB 3 - controller automatically performs reset
            break;
        case PLS_POLLING:
            // USB 2 - Reset Port
            xhci_write_opt(xhci,XHCI_OPT_PORTSC(port_id - 1),portsc | PORTSC_PR);
            break;
        default:
            return K_UDF_BEHAVIOR;
    }
    while (1)
    {
        portsc = xhci_read_opt(xhci,XHCI_OPT_PORTSC(port_id - 1));
        if (!GET_FIELD(portsc,PORTSC_CCS))
        {
            return K_ERROR;
        }
        if (GET_FIELD(portsc,PORTSC_PED))
        {
            // Success
            break;
        }
    }
    return K_SUCCESS;
}

PRIVATE status_t xhci_enable_slot(xhci_t *xhci)
{
    xhci_trb_t trb;
    memset(&trb,0,sizeof(trb));
    trb.flags = TRB_3_TYPE(TRB_TYPE_ENABLE_SLOT);
    xhci_submit_command(xhci,&trb);
    return K_SUCCESS;
}

PRIVATE status_t xhci_configure_port(xhci_t *xhci,uint8_t port_id)
{
    status_t status = xhci_hub_port_reset(xhci,port_id);
    if (ERROR(status))
    {
        pr_log("\3 Port Reset Failed.\n");
        return status;
    }
    // TODO: configure slot
    status = xhci_enable_slot(xhci);
    return K_SUCCESS;
}

PRIVATE status_t port_change_event(xhci_t *xhci,xhci_trb_t *trb)
{
    uint8_t port_id = (trb->addr >> 24) & 0xff;
    pr_log("\2 Port Status Change: port id: %d.",port_id);
    uint32_t portsc;
    portsc = xhci_read_opt(xhci,XHCI_OPT_PORTSC(port_id - 1));

    // Clear PSC bit.
    uint32_t portsc_clear;
    portsc_clear = ((portsc & ~(PORTSC_PED | PORTSC_PR))
                    & ~(PORTSC_PLS_MASK << PORTSC_PLS_SHIFT))
                    | 1 << PORTSC_PLS_SHIFT;
    xhci_write_opt(xhci,XHCI_OPT_PORTSC(port_id - 1),portsc_clear);

    uint8_t is_connected = GET_FIELD(portsc,PORTSC_CCS);

    pr_log("%s",is_connected ? "attach" : "detach");
    pr_log("\n");
    if (is_connected)
    {
        status_t status = xhci_configure_port(xhci,port_id);
        if (ERROR(status))
        {
            return status;
        }
    }
    return K_SUCCESS;
}

PRIVATE void command_completion_event(xhci_t *xhci,xhci_trb_t *trb)
{
    (void)xhci;
    xhci_trb_t *rtrb = (void*)(trb->addr & 0xffffffff);
    pr_log("\1 Command completion.rtrb: %p.\n",rtrb);
    return;
}

PUBLIC void process_event(xhci_t *xhci)
{
    uint16_t i         = xhci->event_ring.dequeue_index;
    uint8_t  cycle_bit = xhci->event_ring.cycle_bit;
    while(1)
    {
        uint32_t temp = xhci->event_ring.ring[i].flags;
        uint8_t event_type = GET_FIELD(temp,TRB_3_TYPE);
        if (cycle_bit != (temp & 1))
        {
            return;
        }
        switch (event_type)
        {
            case TRB_TYPE_PORT_STATUS_CHANGE:
                port_change_event(xhci,&xhci->event_ring.ring[i]);
                break;
            case TRB_TYPE_COMMAND_COMPLETION:
                command_completion_event(xhci,&xhci->event_ring.ring[i]);
                break;
            default:
                pr_log("\2 Event type: %s.\n",trb_type_str(event_type));
                break;
        }
        i++;
        if (i == XHCI_MAX_EVENTS)
        {
            i = 0;
            cycle_bit ^= 1;
        }
        xhci->event_ring.dequeue_index = i;
        xhci->event_ring.cycle_bit = cycle_bit;

        uint64_t addr = xhci->erst->rs_addr + i * sizeof(xhci_trb_t);
        xhci_write_run(xhci,XHCI_IRS_ERDP_LO(0),addr & 0xffffffff);
        xhci_write_run(xhci,XHCI_IRS_ERDP_HI(0),addr >> 32);
    }
}