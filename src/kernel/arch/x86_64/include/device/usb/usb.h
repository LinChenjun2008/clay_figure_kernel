#ifndef __USB_H__
#define __USB_H__

typedef struct usb_hub_s usb_hub_t;
typedef struct usb_dev_s usb_dev_t;

typedef struct usb_hub_opt_s
{
    int (*detect)(usb_hub_t* hub,uint8_t port);
    int (*reset)(usb_hub_t* hub,uint8_t port);
    int (*portmap)(usb_hub_t* hub,uint8_t port);
    int (*disconnect)(usb_hub_t* hub,uint8_t port);
} usb_hub_opt_t;

struct usb_hub_s
{
    usb_hub_opt_t op;
};

struct usb_dev_s
{
    usb_hub_t *hub;
};

PUBLIC const char* trb_type_str(uint8_t trb_type);
PUBLIC const char* port_link_status_str(uint8_t pls);
PUBLIC void process_event(xhci_t *xhci);
PUBLIC status_t reset_port(uint8_t port_id);
PUBLIC status_t configure_port(uint8_t port_id);

#endif