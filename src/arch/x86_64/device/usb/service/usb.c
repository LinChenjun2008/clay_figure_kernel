/*
   Copyright 2024 LinChenjun

   本程序是自由软件
   修改和/或再分发依照 GNU GPL version 3 (or any later version)

*/

#include <kernel/global.h>
#include <kernel/syscall.h>       // inform_intr,sys_send_recv
#include <service.h>              // TICK_SLEEP
#include <device/pci.h>           // pci_device_t,pci functions
#include <device/usb/xhci.h>      // xhci_t
#include <device/usb/xhci_regs.h> // xhci registers
#include <device/usb/xhci_trb.h>  // xhci_trb_t
#include <device/usb/xhci_mem.h> // xhci memory
#include <device/usb/usb.h>       // process_event
#include <task/task.h>            // task_start
#include <std/string.h>           // memset
#include <mem/mem.h>              // pmalloc

#include <log.h>

extern xhci_t *xhci_set;

PRIVATE status_t xhci_hub_port_reset(xhci_t *xhci,uint8_t port_id)
{
    uint32_t portsc = xhci_read_opt(xhci,XHCI_OPT_PORTSC(port_id - 1));
    if (!GET_FIELD(portsc,PORTSC_CCS))
    {
        xhci_write_opt(xhci,XHCI_OPT_PORTSC(port_id - 1),portsc | PORTSC_PR);
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
            portsc |= PORTSC_PR;
            xhci_write_opt(xhci,XHCI_OPT_PORTSC(port_id - 1),portsc);
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
    trb.flags = TRB_3_TYPE(TRB_TYPE_ENABLE_SLOT);
    status_t status;
    status = xhci_submit_command(xhci,&trb);

    *slot_id = (trb.flags >> 24) & 0xff;
    return status;
}

PRIVATE status_t xhci_create_device_context(xhci_t *xhci,uint8_t slot_id)
{
    uint64_t dev_cxt_size = sizeof(xhci_device_cxt32_t);
    if (xhci->csz)
    {
        dev_cxt_size = sizeof(xhci_device_cxt64_t);
    }
    void *cxt;
    status_t status;
    status = pmalloc(dev_cxt_size,
                     XHCI_DEVICE_CONTEXT_ALIGNMENT,
                     XHCI_DEVICE_CONTEXT_BOUNDARY,
                     &cxt);
    if (ERROR(status))
    {
        pr_log("\3 Failed to alloc device context.\n");
        return status;
    }
    xhci->dcbaa[slot_id]       = (uint64_t)cxt;
    xhci->dcbaa_vaddr[slot_id] = (uint64_t)KADDR_P2V(cxt);
    return K_SUCCESS;
}

PRIVATE status_t xhci_setup_device(xhci_t *xhci,uint8_t port_id)
{
    uint8_t port = port_id - 1;

    uint32_t portsc;
    portsc = xhci_read_opt(xhci,XHCI_OPT_PORTSC(port));

    uint8_t speed = GET_FIELD(portsc,PORTSC_SPEED);
    uint16_t max_packet_size = xhci_get_max_init_packet_size(speed);
    (void)max_packet_size;

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
    }
    xhci_create_device_context(xhci,slot_id);
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
        // if (msg.src == usb_event_pid)
        // {
        //     process_connection_event(&msg);
        // }
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