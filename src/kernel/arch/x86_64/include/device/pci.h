#ifndef __PCI_H__
#define __PCI_H__

typedef struct
{
    uint8_t  bus;
    uint8_t  device;
    uint8_t  func;
    uint8_t  header_type;
    uint32_t class_code;
} pci_device_t;

PUBLIC void pci_scan_all_bus();

#endif