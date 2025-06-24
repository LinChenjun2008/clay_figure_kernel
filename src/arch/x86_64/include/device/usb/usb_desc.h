#ifndef __USB_DESC_H__
#define __USB_DESC_H__

#pragma pack(1)

typedef struct usb_desc_header_s
{
    uint8_t bLength;
    uint8_t bDescriptorType;
} usb_desc_header_t;

STATIC_ASSERT(sizeof(usb_desc_header_t) == 2, "");

typedef struct usb_device_desc_s
{
    usb_desc_header_t header;
    uint16_t          bcdUsb;
    uint8_t           bDeviceClass;
    uint8_t           bDeviceSubClass;
    uint8_t           bDeviceProtocol;
    uint8_t           bMaxPacketSize0;
    uint16_t          idVendor;
    uint16_t          idProduct;
    uint16_t          bcdDevice;
    uint8_t           iManufacturer;
    uint8_t           iProduct;
    uint8_t           iSerialNumber;
    uint8_t           bNumConfigurations;
} usb_device_desc_t;

STATIC_ASSERT(sizeof(usb_device_desc_t) == 18, "");

#pragma pack()

#endif