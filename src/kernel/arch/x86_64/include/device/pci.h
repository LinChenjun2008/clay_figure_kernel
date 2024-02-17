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

PUBLIC uint32_t pci_dev_config_read(pci_device_t *dev,uint8_t offset);
PUBLIC void pci_dev_config_write(pci_device_t *dev,uint8_t offset,uint32_t value);
PUBLIC uint64_t pci_dev_read_bar(pci_device_t *dev,uint8_t bar_index);
PUBLIC uint8_t pci_dev_read_cap_point(pci_device_t *dev);

PUBLIC void pci_scan_all_bus();
PUBLIC pci_device_t* pci_dev_match(uint8_t base_class,uint8_t sub_class,uint8_t prog_if);

#endif