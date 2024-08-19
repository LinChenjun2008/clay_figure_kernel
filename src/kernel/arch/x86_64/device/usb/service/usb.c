#include <kernel/global.h>
#include <kernel/syscall.h>
#include <service.h>
#include <device/usb/xhci.h>
#include <device/usb/usb.h>
#include <std/string.h>
#include <task/task.h>

#include <log.h>

extern xhci_t xhci;

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

PUBLIC const char* port_link_status_str(uint8_t pls)
{
    switch (pls)
    {
        case PLS_U0: return "U0";
        case PLS_U1: return "U1";
        case PLS_U2: return "U2";
        case PLS_U3: return "U3";
        case PLS_DISABLED: return "DISABLED";
        case PLS_RX_DETECT: return "RX_DETECT";
        case PLS_INACTIVE: return "INACTIVE";
        case PLS_POLLING: return "POLLING";
        case PLS_RECOVERY: return "RECOVERY";
        case PLS_HOT_RESET: return "HOT_RESET";
        case PLS_COMPILANCE_MODE: return "COMPILANCE_MODE";
        case PLS_TEST_MODE: return "TEST_MODE";
        case PLS_RESUME: return "RESUME";
    }
    return "Unknow PLS";
}

// PRIVATE status_t enable_slot()
// {
//     xhci_trb_t trb;
//     trb.addr   = 0;
//     trb.status = 0;
//     trb.flags  = TRB_3_TYPE(TRB_TYPE_ENABLE_SLOT);
//     xhci_do_command(&trb);
//     return K_SUCCESS;
// }

// PUBLIC status_t configure_port(uint8_t port_id)
// {
//     status_t status = K_SUCCESS;
//     status = reset_port(port_id);
//     if (ERROR(status))
//     {
//         return status;
//     }
//     // Secton 4.3.4
//     // Enable Slot
//     enable_slot();
//     return K_SUCCESS;
// }

PRIVATE void usb_event_task()
{
    while(1)
    {
        process_event();
    };
}

PRIVATE void process_intr()
{
    uint32_t usbsts = xhci_read_opt(XHCI_OPT_USBSTS);
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

// PRIVATE void xhci_port_init()
// {
//     uint64_t i;
//     for (i = 0;i < xhci.max_ports;i++)
//     {
//         uint32_t portsc = xhci_read_opt(XHCI_OPT_PORTSC(i));
//         pr_log("\1 Port %d: PLS: %s.",
//             i + 1,
//             port_link_status_str(GET_FIELD(portsc,PORTSC_PLS)));
//         uint8_t is_connected = portsc & PORTSC_CCS;
//         pr_log("CSC: %d.", GET_FIELD(portsc,PORTSC_CSC));
//         pr_log("is_connected: %s.",is_connected ? "true" : "false");
//         pr_log(" speed: %d, %s%s",
//                 GET_FIELD(portsc,PORTSC_SPEED),
//                 GET_FIELD(portsc,PORTSC_PED) ? "enabled " : "",
//                 GET_FIELD(portsc,PORTSC_PP) ? "powered." : ".");
//         // if (is_connected)
//         // {
//         //     status_t status = configure_port(i + 1);
//         //     pr_log(" %s enable Port.",ERROR(status) ? "Failed in" : "Succeed in");
//         // }
//         pr_log("\n");
//     }
// }


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

// PRIVATE bool xhci_hub_detect(uint32_t port_id)
// {
//     uint32_t portsc = xhci_read_opt(XHCI_OPT_PORTSC(port_id - 1));
//     return GET_FIELD(portsc,PORTSC_CCS) ? 1 : 0;
// }

PUBLIC status_t xhci_hub_reset(uint8_t port_id)
{
    uint32_t portsc = xhci_read_opt(XHCI_OPT_PORTSC(port_id - 1));
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
            xhci_write_opt(XHCI_OPT_PORTSC(port_id - 1),portsc | PORTSC_PR);
            break;
        default:
            return K_ERROR;
    }
    while (1)
    {
        portsc = xhci_read_opt(XHCI_OPT_PORTSC(port_id - 1));
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

// PRIVATE void usb_set_address()
// {
//     message_t msg;
//     msg.type = TICK_SLEEP;
//     msg.m3.l1 = 1;
//     sys_send_recv(NR_BOTH,TICK,&msg);
// }

// PRIVATE void xhci_hub_port_setup(uint16_t port_id)
// {
//     if (!xhci_hub_detect(port_id))
//     {
//         return;
//     }
//     pr_log("\1 device detected: %d.\n",port_id);
//     if(ERROR(xhci_hub_reset(port_id)))
//     {
//         return;
//     }
//     pr_log("\2 port reset: %d.\n",port_id);
//     usb_set_address();
// }

// PRIVATE void xhci_enumerate()
// {
//     uint32_t i;
//     for (i = 0;i < xhci.max_ports;i++)
//     {
//         xhci_hub_port_setup(i + 1);
//     }
// }

PUBLIC void usb_main()
{
    pr_log("\1 USB Service Start.\n");
    xhci_init();
    uint32_t i;
    for (i = 0;i < xhci.max_ports;i++)
    {
        uint32_t portsc = xhci_read_opt(XHCI_OPT_PORTSC(i));
        xhci_write_opt(XHCI_OPT_PORTSC(i),portsc | PORTSC_PR);
    }

    message_t msg;
    msg.type = TICK_SLEEP;
    msg.m3.l1 = 5;
    sys_send_recv(NR_BOTH,TICK,&msg);

    xhci.event_task = task_start("USB Event",
                                 DEFAULT_PRIORITY,
                                 4096,
                                 usb_event_task,
                                 0)->pid;
    // while (1)
    // {
    //     xhci_enumerate();
    // }
    // xhci_port_init();
    while (1)
    {
        sys_send_recv(NR_RECV,RECV_FROM_ANY,&msg);
        switch (msg.type)
        {
        case RECV_FROM_INT:
            process_intr();
            break;
        default:
            break;
        }
    }
}