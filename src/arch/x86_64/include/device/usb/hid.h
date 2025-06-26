#ifndef __HID_H__
#define __HID_H__

#define USB_INTERFACE_SUBCLASS_BOOT     1
#define USB_INTERFACE_PROTOCOL_KEYBOARD 1
#define USB_INTERFACE_PROTOCOL_MOUSE    2

#define HID_REQ_GET_REPORT   0x01
#define HID_REQ_GET_IDLE     0x02
#define HID_REQ_GET_PROTOCOL 0x03
#define HID_REQ_SET_REPORT   0x09
#define HID_REQ_SET_IDLE     0x0A
#define HID_REQ_SET_PROTOCOL 0x0B

#include <device/usb/usb.h> // usb_devie_t

PUBLIC int usb_hid_setup(usb_device_t *usb_dev);

#endif