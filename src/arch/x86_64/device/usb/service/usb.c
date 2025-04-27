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
#include <device/usb/usb.h>       // process_event
#include <task/task.h>            // task_start

#include <log.h>

extern xhci_t *xhci_set;

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

PRIVATE void usb_event_task(void)
{
    uint32_t number_of_xhci = pci_dev_count(0x0c,0x03,0x30);
    while(1)
    {
        uint32_t i;
        for (i = 0;i < number_of_xhci;i++)
        {
            process_intr(&xhci_set[i]);
            process_event(&xhci_set[i]);
        }
        task_yield();
    };
}

PUBLIC void usb_main(void)
{
    pr_log("\1 USB Service Start.\n");
    xhci_setup();
    xhci_start();

    uint32_t number_of_xhci = pci_dev_count(0x0c,0x03,0x30);

    uint32_t i,port;
    for (i = 0;i < number_of_xhci;i++)
    {
        for (port = 0;port < xhci_set[i].max_ports;port++)
        {
            uint32_t portsc = xhci_read_opt(&xhci_set[i],XHCI_OPT_PORTSC(port));
            xhci_write_opt(&xhci_set[i],XHCI_OPT_PORTSC(port),portsc | PORTSC_PR);
        }
    }

    message_t msg;
    msg.type = TICK_SLEEP;
    msg.m3.l1 = 50;
    sys_send_recv(NR_BOTH,TICK,&msg);

    task_start("USB Event",
                DEFAULT_PRIORITY,
                4096,
                usb_event_task,
                0);

    while (1)
    {
        sys_send_recv(NR_RECV,RECV_FROM_ANY,&msg);
        // switch (msg.type)
        // {
        // case RECV_FROM_INT:
        //     break;
        // default:
        //     break;
        // }
    }
}