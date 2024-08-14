#ifndef __USB_H__
#define __USB_H__

PUBLIC char* trb_type_str(uint8_t trb_type);
PUBLIC void process_event();
PUBLIC status_t reset_port(uint8_t port_id);
PUBLIC status_t configure_port(uint8_t port_id);

#endif