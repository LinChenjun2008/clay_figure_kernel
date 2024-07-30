#include <kernel/global.h>
#include <kernel/syscall.h>
#include <service.h>
#include <device/usb/xhci.h>
#include <std/string.h>
#include <task/task.h>

#include <log.h>

extern xhci_t xhci;

PRIVATE char* trb_type_str(uint8_t trb_type)
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

// PRIVATE void enable_slot(uint8_t *slot)
// {
//     xhci_trb_t trb;
//     trb.addr = 0;
//     trb.status = 0;
//     trb.flags = TRB_3_TYPE(TRB_TYPE_ENABLE_SLOT);
//     xhci_do_command(&trb);
//     *slot = GET_TRB_3_SLOT(trb.flags);
// }

// PRIVATE void port_attach(uint8_t port_id)
// {
//     // xhci_write_opt_reg32(XHCI_OPT_REG_PORTSC(port_id - 1),portsc | PORTSC_PR);
//     uint32_t portsc = xhci_read_opt_reg32(XHCI_OPT_REG_PORTSC(port_id));
//     uint8_t ped = GET_PORTSC_PED(portsc);
//     uint8_t pls = GET_PORTSC_PLS(portsc);
//     pr_log("Port: %d, PED: %d,PLS %d.\n",port_id,ped,pls);

//     if (pls == 7) // USB 2,polling
//     {
//         xhci_write_opt_reg32(XHCI_OPT_REG_PORTSC(port_id),portsc | PORTSC_PR);
//     }
//     // return;
//     // Enable Slot
//     uint8_t slot;
//     pr_log("enable.\n");
//     enable_slot(&slot);
//     pr_log("slot enable.slot: %d\n",slot);
// }

// PRIVATE void port_status_change_event(xhci_trb_t *event)
// {
//     uint8_t port_id = (event->addr >> 24) & 0xff;
//     uint8_t completion_code = (event->flags >> 24) & 0xff;
//     pr_log("\1 Port Status Change Event. port_id: %02x. completion code: %02x.\n",port_id,completion_code);
//     // System software then reads the PORTSC register of the port that
//     // generated the event.
//     // CSC = ‘1’ if the event was due to an attach (CCS = ‘1’) or detach (CCS = ‘0’).
//     // Assuming the event was due to an attach

//     uint32_t portsc = xhci_read_opt_reg32(XHCI_OPT_REG_PORTSC(port_id));
//     if (portsc & PORTSC_CSC)
//     {
//         pr_log("\1 device %s.\n",portsc & PORTSC_CCS ? "attach" : "detach");
//     }
//     portsc & PORTSC_CSC ? port_attach(port_id - 1) :0;
//     return;
// }

PRIVATE void process_event()
{
    uint16_t i = xhci.event_index;
    uint8_t event_cycle_bit = xhci.event_cycle_bit;
    uint8_t t = 1;
    while(1)
    {
        uint32_t temp = xhci.event_ring[i].flags;
        uint8_t event_type = GET_TRB_3_TYPE(temp);
        uint8_t cycle_bit = temp & 1;// cycle bit
        if (event_cycle_bit != cycle_bit)
        {
            break;
        }
        pr_log("\1 process_event(): trb type: %s.\n",
               trb_type_str(event_type));
        switch (event_type)
        {
            case TRB_TYPE_PORT_STATUS_CHANGE:
                pr_log("\1 Port Status Change Event.\n");
                break;
            case TRB_TYPE_COMMAND_COMPLETION:
                if (xhci.command_addr == xhci.event_ring[i].addr)
                {
                    xhci.command_result[0] = xhci.event_ring[i].status;
                    xhci.command_result[1] = xhci.event_ring[i].flags;
                }
                else
                {}
                xhci.command_completion = TRUE;
                break;
            default:
                pr_log("\1 Unknow Event: %d.\n",(temp >> 10) & 0x3f);
                break;
        }
        i++;
        if (i == XHCI_MAX_EVENTS)
        {
            i = 0;
            event_cycle_bit ^= 1;
            if (t == 0)
            {
                break;
            }
            t--;
        }
    }
    xhci.event_index = i;
    xhci.event_cycle_bit = event_cycle_bit;

    uint64_t addr = xhci.erst->rs_addr + i * sizeof(xhci_trb_t);
    addr |= (1 << 3); // ERDP busy
    xhci_write_run_reg32(XHCI_IRS_ERDP_LO(0),addr & 0xffffffff);
    xhci_write_run_reg32(XHCI_IRS_ERDP_HI(0),addr >> 32);
}

PRIVATE void usb_event_task()
{
    while(1)
    {
        process_event();
    };
}

PRIVATE void usb_polling_task()
{
    while(1)
    {
        uint16_t port;
        for (port = 0;port < xhci.max_ports;port++)
        {
            uint32_t portsc = xhci_read_opt_reg32(XHCI_OPT_REG_PORTSC(port));
            if (GET_PORTSC_CSC(portsc) == 1)
            {
                if (GET_PORTSC_CCS(portsc) == 1)
                {
                    pr_log("\1 port %d attach.",port);
                    pr_log("speed: %d ped: %d prc: %d pls: %d\n",
                            GET_PORTSC_SPEED(portsc),
                            GET_PORTSC_PED(portsc),
                            GET_PORTSC_PRC(portsc),
                            GET_PORTSC_PLS(portsc));
                    if (GET_PORTSC_PLS(portsc) == 7)
                    {
                        // reset port to enable it.
                        xhci_write_opt_reg32(XHCI_OPT_REG_PORTSC(port),PORTSC_PR);
                        int i = 0xffffff;
                        while(--i);
                        portsc = xhci_read_opt_reg32(XHCI_OPT_REG_PORTSC(port));
                        pr_log("Port Reset. speed: %d ped: %d prc: %d pls: %d\n",
                            GET_PORTSC_SPEED(portsc),
                            GET_PORTSC_PED(portsc),
                            GET_PORTSC_PRC(portsc),
                            GET_PORTSC_PLS(portsc));
                    }
                }
                if (GET_PORTSC_CCS(portsc) == 0)
                {
                    pr_log("\1 port %d detach.\n",port);
                }
                xhci_write_opt_reg32(XHCI_OPT_REG_PORTSC(port),portsc | PORTSC_CSC);
                // pr_log("\1 reset port: %d.\n",port);
                // xhci_write_opt_reg32(XHCI_OPT_REG_PORTSC(port),portsc | PORTSC_PR);
                // uint32_t portsc = xhci_read_opt_reg32(XHCI_OPT_REG_PORTSC(port));
                // pr_log("\1 port %d speed: %d ped: %d prc: %d pls: %d\n",
                //         port,
                //         GET_PORTSC_SPEED(portsc),
                //         GET_PORTSC_PED(portsc),
                //         GET_PORTSC_PRC(portsc),
                //         GET_PORTSC_PLS(portsc));
            }
        }
    };
}


PUBLIC void usb_main()
{
    pr_log("\1 USB Service Start.\n");
    xhci_init();
    xhci.event_task = task_start("USB Event Task",DEFAULT_PRIORITY,4096,usb_event_task,0)->pid;
    task_start("USB Port Polling Task",DEFAULT_PRIORITY,4096,usb_polling_task,0);
    message_t msg;
    while (1)
    {
        sys_send_recv(NR_RECV,RECV_FROM_ANY,&msg);
        switch (msg.type)
        {
            case RECV_FROM_INT:
                uint32_t usbsts = xhci_read_opt_reg32(XHCI_OPT_REG_USBSTS);
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
                break;
            default:
                break;
        }
    }
}