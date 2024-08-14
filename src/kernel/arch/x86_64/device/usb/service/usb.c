#include <kernel/global.h>
#include <kernel/syscall.h>
#include <service.h>
#include <device/usb/xhci.h>
#include <device/usb/usb.h>
#include <std/string.h>
#include <task/task.h>

#include <log.h>

extern xhci_t xhci;

PUBLIC char* trb_type_str(uint8_t trb_type)
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

PRIVATE status_t enable_slot()
{
    xhci_trb_t trb;
    trb.addr   = 0;
    trb.status = 0;
    trb.flags  = TRB_3_TYPE(TRB_TYPE_ENABLE_SLOT);
    xhci_do_command(&trb);
    return K_SUCCESS;
}

PUBLIC status_t configure_port(uint8_t port_id)
{
    status_t status = K_SUCCESS;
    status = reset_port(port_id);
    if (ERROR(status))
    {
        return status;
    }
    // Secton 4.3.4
    // Enable Slot
    pr_log("\1 Enable Slot.\n");
    enable_slot();
    return K_SUCCESS;
}

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

extern volatile uint64_t global_ticks;

PUBLIC void usb_main()
{
    pr_log("\1 USB Service Start.\n");
    xhci_init();
    xhci.event_task = task_start("USB Event",
                                 DEFAULT_PRIORITY,
                                 4096,
                                 usb_event_task,
                                 0)->pid;
    // xhci_enumerate();
    message_t msg;
    msg.type = TICK_SLEEP;
    msg.m3.l1 = 5;
    sys_send_recv(NR_BOTH,TICK,&msg);

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