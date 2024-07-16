#include <kernel/global.h>
#include <kernel/syscall.h>
#include <service.h>

#include <device/usb/hci/xhci.h>
#include <std/string.h>

#include <log.h>

extern xhci_t xhci;

uint8_t arr[256];
PUBLIC void usb_main()
{
    memset(arr,0xff,256);
    pr_log("\1 USB Service Start.\n");
    uint32_t i = 0;
    while (1)
    {
        for (i = 0;i < (xhci.cap_regs->HCCPARAMS1 & MAX_SLOTS);i++)
        {
            xhci_usb_port_regs_t *port = (void*)((uintptr_t)xhci.opt_regs + 0x400 + 0x10 * i);
            if (port->PORTSC & CSC)
            {
                if ((port->PORTSC & CCS) != arr[i])
                {
                    if ((port->PORTSC & CCS) == 1)
                    {
                        pr_log("\2 CSC == 1 && CCS == 1(attach)(%d)\n",i + 1);
                        pr_log("\1 if successful,PED == 1 PLS and PP == 0(USB3),or PED == 0,PLS == 7(polling)(USB2).\n");
                        pr_log("PED: %x\n",(port->PORTSC & PED) >> 1);
                        pr_log("PLS: %x\n",(port->PORTSC & PLS) >> 5);
                    }
                    if ((port->PORTSC & CCS) == 0)
                    {
                        pr_log("\2 CSC == 1 && CCS == 0(detach)(%d)\n",i + 1);
                        pr_log("PED: %x\n",(port->PORTSC & PED) >> 1);
                        pr_log("PLS: %x\n",(port->PORTSC & PLS) >> 5);
                    }
                    arr[i] = (port->PORTSC & CCS);
                }

            }
        }
    }
}