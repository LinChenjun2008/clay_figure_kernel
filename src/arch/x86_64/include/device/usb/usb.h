/*
   Copyright 2024 LinChenjun

   本程序是自由软件
   修改和/或再分发依照 GNU GPL version 3 (or any later version)

*/

#ifndef __USB_H__
#define __USB_H__

PUBLIC const char* trb_type_str(uint8_t trb_type);
PUBLIC const char* port_link_status_str(uint8_t pls);
PUBLIC void process_event(xhci_t *xhci);
PUBLIC status_t reset_port(uint8_t port_id);

#endif