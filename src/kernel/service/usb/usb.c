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

    while (1)
    {
        // // intr_reg_set_t *queue_pointer = (intr_reg_set_t*)(xhci.intr_reg_sets->ERDP & ~0xf);
        // // trb_t *trb = (trb_t*)queue_pointer;
        // // if ((trb->status & 1) /*cycle_bit*/ == xhci.er.cycle_bit)
        // // {
        // //     trb_t *event_trb = trb;
        // //     // trb_type
        // //     switch ((event_trb->status >> 10) & 0x3f)
        // //     {
        // //         case 34: // Port Status Change Event
        // //             pr_log("\2 Port Status Change Event: port id = %d\n",event_trb->address >> 24);
        // //             break;
        // //         default:
        // //             pr_log("\2 Event Ring: port id = %d\n",event_trb->address >> 24);
        // //             break;
        // //     }
        // // }

        // uint32_t i = 0;
        // for (i = 0;i < (xhci.cap_regs->HCCPARAMS1 & XHCI_HCSPARAMS1_MAX_SLOTS);i++)
        // {
        //     xhci_usb_port_regs_t *port = (void*)((addr_t)xhci.opt_regs + 0x400 + 0x10 * i);
        //     if (port->PORTSC & XHCI_PORTSC_CSC)
        //     {
        //         if ((port->PORTSC & XHCI_PORTSC_CCS) != arr[i])
        //         {
        //             if ((port->PORTSC & XHCI_PORTSC_CCS) == 1)
        //             {
        //                 pr_log("\2 CSC == 1 && CCS == 1(attach)(%d)\n",i + 1);
        //                 pr_log("\1 if successful,PED == 1 PLS and PP == 0(USB3),or PED == 0,PLS == 7(polling)(USB2).\n");
        //                 pr_log("PED: %x\n",(port->PORTSC & XHCI_PORTSC_PED) >> 1);
        //                 pr_log("PLS: %x\n",(port->PORTSC & XHCI_PORTSC_PLS) >> 5);
        //             }
        //             if ((port->PORTSC & XHCI_PORTSC_CCS) == 0)
        //             {
        //                 pr_log("\2 CSC == 1 && CCS == 0(detach)(%d)\n",i + 1);
        //                 pr_log("PED: %x\n",(port->PORTSC & XHCI_PORTSC_PED) >> 1);
        //                 pr_log("PLS: %x\n",(port->PORTSC & XHCI_PORTSC_PLS) >> 5);
        //             }
        //             pr_log("\1 USBCMD: 0x%x\n",xhci.opt_regs->USBCMD);
        //             pr_log("\1 USBSTS: 0x%x\n",xhci.opt_regs->USBSTS);
        //             arr[i] = (port->PORTSC & XHCI_PORTSC_CCS);
        //         }

        //     }
        // }
    }
}