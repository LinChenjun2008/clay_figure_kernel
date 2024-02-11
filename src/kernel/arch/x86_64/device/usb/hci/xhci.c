#include <kernel/global.h>
#include <device/pci.h>

#include <log.h>

PUBLIC void xhci_init()
{
    pci_device_t *device = pci_dev_match(0x0c,0x03,0x30);
    if (device == NULL)
    {
        return;
    }
    pr_log("\1xhci_dev vendor id :%x \n",
    pci_dev_config_read(device,0) & 0xffff);

    int i;
    for (i = 0;i < 0x20;i+=4)
    {
        pr_log("\1%02x: %08x\n",i,pci_dev_config_read(device,i));
    }
    pr_log("\1bar0 %016x\n",pci_read_bar(device,0));
    pr_log("\1bar1 %016x\n",pci_read_bar(device,1));
}