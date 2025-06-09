/*
   Copyright 2024-2025 LinChenjun

   本程序是自由软件
   修改和/或再分发依照 GNU GPL version 3 (or any later version)

*/

#ifndef __USB_H__
#define __USB_H__

#include <device/spinlock.h> // spinlock_t

typedef struct pci_device_s pci_device_t;

typedef struct usb_pipe_s usb_pipe_t;
typedef struct usb_hub_s usb_hub_t;

typedef struct usb_config_descriptor_s usb_config_descriptor_t;
typedef struct usb_interface_descriptor_s usb_interface_descriptor_t;

// Common information for usb controllers.
typedef struct usb_s
{
    usb_pipe_t   *freelist;
    spinlock_t    resetlock;
    pci_device_t *pci;
    void         *mmio;
    uint8_t       type;
    uint8_t       max_addr;
} usb_t;

// Information on a USB end point.
struct usb_pipe_s
{
    union
    {
        usb_t      *ctrl;
        usb_pipe_t *freenext;
    };
    uint8_t  type;
    uint8_t  ep;
    uint8_t  dev_addr;
    uint8_t  speed;
    uint16_t max_packet;
    uint8_t  ep_type;
};

// Common information for usb devices.
typedef struct usb_device_s
{
    usb_hub_t                  *hub;
    usb_pipe_t                 *defpipe;
    uint32_t                    port;
    usb_config_descriptor_t    *config;
    usb_interface_descriptor_t *iface;
    int                         imax;
    uint8_t                     speed;
    uint8_t                     dev_addr;
} usb_device_t;

typedef int (*usb_hub_op_detect    ) (usb_hub_t *hub,uint32_t port);
typedef int (*usb_hub_op_reset     ) (usb_hub_t *hub,uint32_t port);
typedef int (*usb_hub_op_portmap   ) (usb_hub_t *hub,uint32_t port);
typedef int (*usb_hub_op_disconnect) (usb_hub_t *hub,uint32_t port);

// Hub callback (32bit) info
typedef struct usb_hub_op_s
{
    int (*detect    ) (usb_hub_t *hub,uint32_t port);
    int (*reset     ) (usb_hub_t *hub,uint32_t port);
    int (*portmap   ) (usb_hub_t *hub,uint32_t port);
    int (*disconnect) (usb_hub_t *hub,uint32_t port);
} usb_hub_op_t;

// Information for enumerating USB hubs
struct usb_hub_s
{
    usb_hub_op_t *op;
    usb_device_t *usb_dev;
    usb_t        *ctrl;
    spinlock_t    lock;
    uint32_t      detectend;
    uint32_t      port;
    uint32_t      threads;
    uint32_t      portcount;
    uint32_t      devcount;
};

typedef struct usb_hub_set_s
{
    usb_hub_t *hubs;
    size_t     count;
} usb_hub_set_t;

typedef struct usb_port_status_change_s
{
    uint8_t port;
    uint8_t is_connect;
} usb_port_connecton_event_t;

#define USB_FULLSPEED  0
#define USB_LOWSPEED   1
#define USB_HIGHSPEED  2
#define USB_SUPERSPEED 3

#define USB_MAXADDR  127

////////////////////////////////////////////////////////////////////////////////
//
// USB structs and flags
//
////////////////////////////////////////////////////////////////////////////////

// USB mandated timings (in ms)
#define USB_TIME_SIGATT 100
#define USB_TIME_ATTDB  100
#define USB_TIME_DRST   10
#define USB_TIME_DRSTR  50
#define USB_TIME_RSTRCY 10

#define USB_TIME_STATUS  50
#define USB_TIME_DATAIN  500
#define USB_TIME_COMMAND 5000

#define USB_TIME_SETADDR_RECOVERY 2

#define USB_PID_OUT                     0xe1
#define USB_PID_IN                      0x69
#define USB_PID_SETUP                   0x2d

#define USB_DIR_OUT                     0               /* to device */
#define USB_DIR_IN                      0x80            /* to host */

#define USB_TYPE_MASK                   (0x03 << 5)
#define USB_TYPE_STANDARD               (0x00 << 5)
#define USB_TYPE_CLASS                  (0x01 << 5)
#define USB_TYPE_VENDOR                 (0x02 << 5)
#define USB_TYPE_RESERVED               (0x03 << 5)

#define USB_RECIP_MASK                  0x1f
#define USB_RECIP_DEVICE                0x00
#define USB_RECIP_INTERFACE             0x01
#define USB_RECIP_ENDPOINT              0x02
#define USB_RECIP_OTHER                 0x03

#define USB_REQ_GET_STATUS              0x00
#define USB_REQ_CLEAR_FEATURE           0x01
#define USB_REQ_SET_FEATURE             0x03
#define USB_REQ_SET_ADDRESS             0x05
#define USB_REQ_GET_DESCRIPTOR          0x06
#define USB_REQ_SET_DESCRIPTOR          0x07
#define USB_REQ_GET_CONFIGURATION       0x08
#define USB_REQ_SET_CONFIGURATION       0x09
#define USB_REQ_GET_INTERFACE           0x0A
#define USB_REQ_SET_INTERFACE           0x0B
#define USB_REQ_SYNCH_FRAME             0x0C

#pragma pack(1)

typedef struct usb_ctrl_request_s
{
    uint8_t  bRequestType;
    uint8_t  bRequest;
    uint16_t wValue;
    uint16_t wIndex;
    uint16_t wLength;
} usb_ctrl_request_t;

#define USB_DT_DEVICE                   0x01
#define USB_DT_CONFIG                   0x02
#define USB_DT_STRING                   0x03
#define USB_DT_INTERFACE                0x04
#define USB_DT_ENDPOINT                 0x05
#define USB_DT_DEVICE_QUALIFIER         0x06
#define USB_DT_OTHER_SPEED_CONFIG       0x07
#define USB_DT_ENDPOINT_COMPANION       0x30

typedef struct usb_device_descriptor_s
{
    uint8_t  bLength;
    uint8_t  bDescriptorType;

    uint16_t bcdUSB;
    uint8_t  bDeviceClass;
    uint8_t  bDeviceSubClass;
    uint8_t  bDeviceProtocol;
    uint8_t  bMaxPacketSize0;
    uint16_t idVendor;
    uint16_t idProduct;
    uint16_t bcdDevice;
    uint8_t  iManufacturer;
    uint8_t  iProduct;
    uint8_t  iSerialNumber;
    uint8_t  bNumConfigurations;
} usb_device_descriptor_t;

#define USB_CLASS_PER_INTERFACE         0       /* for DeviceClass */
#define USB_CLASS_AUDIO                 1
#define USB_CLASS_COMM                  2
#define USB_CLASS_HID                   3
#define USB_CLASS_PHYSICAL              5
#define USB_CLASS_STILL_IMAGE           6
#define USB_CLASS_PRINTER               7
#define USB_CLASS_MASS_STORAGE          8
#define USB_CLASS_HUB                   9

struct usb_config_descriptor_s
{
    uint8_t  bLength;
    uint8_t  bDescriptorType;

    uint16_t wTotalLength;
    uint8_t  bNumInterfaces;
    uint8_t  bConfigurationValue;
    uint8_t  iConfiguration;
    uint8_t  bmAttributes;
    uint8_t  bMaxPower;
};

struct usb_interface_descriptor_s
{
    uint8_t  bLength;
    uint8_t  bDescriptorType;

    uint8_t  bInterfaceNumber;
    uint8_t  bAlternateSetting;
    uint8_t  bNumEndpoints;
    uint8_t  bInterfaceClass;
    uint8_t  bInterfaceSubClass;
    uint8_t  bInterfaceProtocol;
    uint8_t  iInterface;
};

typedef struct usb_endpoint_descriptor_s
{
    uint8_t  bLength;
    uint8_t  bDescriptorType;

    uint8_t  bEndpointAddress;
    uint8_t  bmAttributes;
    uint16_t wMaxPacketSize;
    uint8_t  bInterval;
} usb_endpoint_descriptor_t;

#define USB_ENDPOINT_NUMBER_MASK        0x0f    /* in bEndpointAddress */
#define USB_ENDPOINT_DIR_MASK           0x80

#define USB_ENDPOINT_XFERTYPE_MASK      0x03    /* in bmAttributes */
#define USB_ENDPOINT_XFER_CONTROL       0
#define USB_ENDPOINT_XFER_ISOC          1
#define USB_ENDPOINT_XFER_BULK          2
#define USB_ENDPOINT_XFER_INT           3
#define USB_ENDPOINT_MAX_ADJUSTABLE     0x80

#define USB_CONTROL_SETUP_SIZE          8

#pragma pack()

////////////////////////////////////////////////////////////////////////////////
//
// usb mass storage flags
//
////////////////////////////////////////////////////////////////////////////////

#define US_SC_ATAPI_8020   0x02
#define US_SC_ATAPI_8070   0x05
#define US_SC_SCSI         0x06

#define US_PR_BULK         0x50  /* bulk-only transport */
#define US_PR_UAS          0x62  /* usb attached scsi   */

PUBLIC usb_pipe_t *usb_realloc_pipe(
    usb_device_t *usb_dev,
    usb_pipe_t *pipe,
    usb_endpoint_descriptor_t *epdesc);

PUBLIC usb_pipe_t *usb_alloc_pipe(
    usb_device_t *usb_dev,
    usb_endpoint_descriptor_t *epdesc);

PUBLIC void usb_free_pipe(usb_device_t *usb_dev,usb_pipe_t *pipe);

PUBLIC int usb_send_default_control(
    usb_pipe_t *pipe,
    const usb_ctrl_request_t *req,
    void *data);

PUBLIC void usb_add_freelist(usb_pipe_t *pipe);

PUBLIC void usb_desc2pipe(
    usb_pipe_t *pipe,
    usb_device_t *usb_dev,
    usb_endpoint_descriptor_t *epdesc);


PUBLIC int usb_get_period(
    usb_device_t *usb_dev,
    usb_endpoint_descriptor_t *epdesc);

#endif