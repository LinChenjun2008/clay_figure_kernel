#include <kernel/global.h>
#include <device/usb/xhci.h>
#include <device/usb/usb.h>

#include <log.h>

extern xhci_t xhci;

PRIVATE void port_change_event(xhci_trb_t *trb)
{
    uint8_t port_id = (trb->addr >> 24) & 0xff;
    pr_log("\2 port id: %d.\n",port_id);

    uint32_t portsc = xhci_read_opt(XHCI_OPT_PORTSC(port_id - 1));
    pr_log("\1 Port %d: PLS: %s.",
        port_id,
        port_link_status_str(GET_FIELD(portsc,PORTSC_PLS)));
    uint8_t is_connected = portsc & PORTSC_CCS;
    pr_log("CSC: %d.", GET_FIELD(portsc,PORTSC_CSC));
    pr_log("is_connected: %s.",is_connected ? "true" : "false");
    pr_log(" speed: %d, %s%s",
            GET_FIELD(portsc,PORTSC_SPEED),
            GET_FIELD(portsc,PORTSC_PED) ? "enabled " : "",
            GET_FIELD(portsc,PORTSC_PP) ? "powered." : ".");
    // if (is_connected)
    // {
    //     status_t status = configure_port(i + 1);
    //     pr_log(" %s enable Port.",ERROR(status) ? "Failed in" : "Succeed in");
    // }
    pr_log("\n");


    // Clear PSC bit.
    // uint32_t portsc = xhci_read_opt(XHCI_OPT_PORTSC(port_id - 1));
    // portsc &= ~(PORTSC_PED | PORTSC_PR);
    // portsc &= 0xf << 5; // PLS Mask
    // portsc |=   1 << 5;
    // xhci_write_opt(XHCI_OPT_PORTSC(port_id - 1),portsc);
    return;
}

PUBLIC void process_event()
{
    uint16_t i         = xhci.event_ring.dequeue_index;
    uint8_t  cycle_bit = xhci.event_ring.cycle_bit;
    while(1)
    {
        uint32_t temp = xhci.event_ring.ring[i].flags;
        uint8_t event_type = GET_FIELD(temp,TRB_3_TYPE);
        if (cycle_bit != (temp & 1))
        {
            return;
        }
        pr_log("\2 Event type: %s.\n",trb_type_str(event_type));
        switch (event_type)
        {
        case TRB_TYPE_PORT_STATUS_CHANGE:
            port_change_event(&xhci.event_ring.ring[i]);
            break;
        case TRB_TYPE_COMMAND_COMPLETION:
            pr_log("\1 command completion.\n");
            break;
        default:
            break;
        }
        i++;
        if (i == XHCI_MAX_EVENTS)
        {
            i = 0;
            cycle_bit ^= 1;
        }
        xhci.event_ring.dequeue_index = i;
        xhci.event_ring.cycle_bit = cycle_bit;

        uint64_t addr = xhci.erst->rs_addr + i * sizeof(xhci_trb_t);
        xhci_write_run(XHCI_IRS_ERDP_LO(0),addr & 0xffffffff);
        xhci_write_run(XHCI_IRS_ERDP_HI(0),addr >> 32);
    }
}