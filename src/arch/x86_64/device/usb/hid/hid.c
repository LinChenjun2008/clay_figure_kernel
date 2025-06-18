#include <kernel/global.h>
#include <device/usb/usb.h> // usb_device_t
#include <device/usb/hid.h> // usb hid flags

#include <log.h>

PUBLIC int usb_hid_setup(usb_device_t *usb_dev)
{
    usb_interface_descriptor_t *iface = usb_dev->iface;
    if (iface->bInterfaceSubClass != USB_INTERFACE_SUBCLASS_BOOT)
    {
        pr_log(LOG_ERROR,"Not support boot protocol.\n");
        return -1;
    }
    usb_endpoint_descriptor_t *epdesc = usb_find_desc(
        usb_dev,
        USB_ENDPOINT_XFER_INT,
        USB_DIR_IN);
    if (epdesc == NULL)
    {
        pr_log(LOG_ERROR,"No usb hid intr.\n");
        return -1;
    }
    /// TODO: setup devices
    if (iface->bInterfaceProtocol == 1)
    {
        pr_log(LOG_DEBUG,"USB Keyboard.\n");
    }
    else if (iface->bInterfaceProtocol == 2)
    {
        pr_log(LOG_DEBUG,"USB Mouse.\n");
    }
    else
    {
        pr_log(LOG_WARN,"Unknow USB HID device.\n");
    }
    return 0;
}