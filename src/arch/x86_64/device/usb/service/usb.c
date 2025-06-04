/*
   Copyright 2024 LinChenjun

   本程序是自由软件
   修改和/或再分发依照 GNU GPL version 3 (or any later version)

*/

#include <kernel/global.h>
#include <service.h>         // previous prototype for 'usb_main'
#include <device/usb/xhci.h> // xhci_setup
#include <device/usb/usb.h>  // previous prototype for 'usb_enumerate'
#include <mem/mem.h>         // pmalloc
#include <std/string.h>      // memset
#include <task/task.h>       // task_msleep,task_start
#include <log.h>

PRIVATE void usb_setup()
{
    xhci_setup();
}

PRIVATE int usb_hub_port_setup(usb_device_t *usb_dev)
{
    usb_hub_t *hub = usb_dev->hub;
    uint32_t port = usb_dev->port;
    // while (1)
    {
        int ret = hub->op->detect(hub,port);
        if (ret == 0)
        {
            // No device connected.
            pr_log("\2 Device disconnect: %d\n",port);
            return 0;
        }
        // task_msleep(5);
    }
    task_msleep(USB_TIME_ATTDB);
    pr_log("\2 Device connect: %d\n",port);

// done:
//     hub->threads--;
    pfree(KADDR_V2P(usb_dev));
    return 1;
}

PUBLIC void usb_enumerate(usb_hub_t *hub)
{
    uint32_t portcount = hub->portcount;
    hub->threads = portcount;
    // Launch task for every port
    uint32_t i = 0;
    for (i = 0;i < portcount;i++)
    {
        usb_device_t *usb_dev;
        status_t status;
        status = pmalloc(sizeof(*usb_dev),16,0,&usb_dev);
        if (ERROR(status))
        {
            pr_log("\3 Failed to alloc usb dev.\n");
            continue;
        }
        usb_dev = KADDR_P2V(usb_dev);
        memset(usb_dev,0,sizeof(*usb_dev));

        usb_dev->hub = hub;
        usb_dev->port = i;

        // task_start(
        //     "USB Port Setup",
        //     DEFAULT_PRIORITY,
        //     4096,
        //     usb_hub_port_setup,
        //     (size_t)usb_dev);
        usb_hub_port_setup(usb_dev);
    }
    // while (hub->threads != 0) task_yield();
    return;
}

PUBLIC void usb_main(void)
{
    usb_setup();
    while (1)
    {
        continue;
    }
}