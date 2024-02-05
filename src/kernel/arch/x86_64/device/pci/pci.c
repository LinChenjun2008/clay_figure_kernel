#include <kernel/global.h>
#include <device/pci.h>
#include <io.h>

#include <log.h>

PRIVATE pci_device_t pci_devices[256];
PRIVATE uint8_t number_of_pci_device;

PRIVATE uint32_t pci_config_read(uint8_t bus, uint8_t dev, uint8_t func, uint8_t offset)
{
    uint32_t address;
    /// PCI_CONFIG_ADDRESS:
    /// 31         | 30 - 24  | 23 - 16    | 15 - 11       | 10 - 8          | 7 - 0           |
    /// Enable bit | Reserved | Bus Number | Device NUmber | Function NUmber | Register offset |

    address = 0x80000000 \
            | (uint32_t)bus << 16 \
            | (uint32_t)dev << 11 \
            | (uint32_t)func << 8 \
            | (offset & 0xfc);

    io_out32(PCI_CONFIG_ADDRESS,address);
    return io_in32(PCI_CONFIG_DATA);
}

PRIVATE uint8_t read_header_type(uint8_t bus, uint8_t dev, uint8_t func)
{
    return (uint8_t)((pci_config_read(bus,dev,func,0x0c) >> 16) & 0xff);
}

PRIVATE uint16_t read_vendor_id(uint8_t bus, uint8_t dev, uint8_t func)
{
    return (uint16_t)(pci_config_read(bus,dev,func,0x00) & 0xffff);
}

PRIVATE uint32_t read_class_code(uint8_t bus, uint8_t dev, uint8_t func)
{
    return pci_config_read(bus,dev,func,0x08);
}

PRIVATE uint8_t read_base_class_code(uint8_t bus, uint8_t dev, uint8_t func)
{
    return (uint8_t)((read_class_code(bus,dev,func) >> 24) & 0xff);
}

PRIVATE uint8_t read_sub_class_code(uint8_t bus, uint8_t dev, uint8_t func)
{
    return (uint8_t)((read_class_code(bus,dev,func) >> 16) & 0xff);
}

PRIVATE uint8_t read_secondary_bus_number(uint8_t bus, uint8_t dev, uint8_t func)
{
    return (uint8_t)((pci_config_read(bus,dev,func,0x18) >> 8) & 0xff);
}

PRIVATE void add_device(pci_device_t *device)
{
    pr_log("\2pci device: %d:",number_of_pci_device);
    pr_log("{bus: %d,dev %d,func %d,header type: %d,class code 0x%x}\n",
        device->bus,device->device,device->func,device->header_type,device->class_code);



    pci_devices[number_of_pci_device] = *device;
    number_of_pci_device++;
}


PRIVATE void scan_func(uint8_t bus,uint8_t dev,uint8_t func);
PRIVATE void scan_device(uint8_t bus,uint8_t dev);
PRIVATE void scan_bus(uint8_t bus);

PRIVATE void scan_func(uint8_t bus,uint8_t dev,uint8_t func)
{
    uint8_t base_class;
    uint8_t sub_class;
    uint8_t secondary_bus;

    base_class = read_base_class_code(bus,dev,func);
    sub_class  = read_sub_class_code(bus,dev,func);

    uint8_t header_type = read_header_type(bus,dev,func);
    uint32_t class_code = read_class_code(bus,dev,func);
    pci_device_t device = {bus,dev,func,header_type,class_code};
    add_device(&device);

    if (base_class == 0x06 && sub_class == 0x04) // PCI to PCI bridge
    {
        secondary_bus = read_secondary_bus_number(bus,dev,func);
        scan_bus(secondary_bus);
    }
}

PRIVATE void scan_device(uint8_t bus,uint8_t dev)
{
    uint8_t func = 0;
    uint16_t vendor_id = read_vendor_id(bus,dev,func);
    if (vendor_id == 0xffff) // device dosen't exist
    {
        return;
    }
    uint8_t header_type = read_header_type(bus,dev,func);
    if ((header_type & 0x80) == 0) // single-function device
    {
        scan_func(bus,dev,0);
        return;
    }
    // multi-functon device
    for (func = 0;func < 8;func++)
    {
        vendor_id = read_vendor_id(bus,dev,func);
        if (vendor_id != 0xffff)
        {
            scan_func(bus,dev,func);
        }
    }
    return;
}

PRIVATE void scan_bus(uint8_t bus)
{
    uint8_t dev;
    for (dev = 0;dev < 32;dev++)
    {
        scan_device(bus,dev);
    }
}

PUBLIC void pci_scan_all_bus()
{
    number_of_pci_device = 0;
    uint8_t func;
    uint8_t header_type = read_header_type(0,0,0);
    if ((header_type & 0x80) == 0)// singal pci host controller
    {
        scan_bus(0);
        return;
    }
    for (func = 0;func < 8;func++)
    {
        if (read_vendor_id(0,0,func) == 0xffff)
        {
            continue;
        }
        // bus = func;
        scan_bus(func);
    }
    return;
}