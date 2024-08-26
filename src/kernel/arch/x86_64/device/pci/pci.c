#include <kernel/global.h>
#include <device/pci.h>
#include <io.h>
#include <device/pic.h>

PRIVATE pci_device_t pci_devices[256];
PRIVATE uint8_t number_of_pci_device;

PRIVATE uint32_t pci_config_read(
    uint8_t bus,
    uint8_t dev,
    uint8_t func,
    uint8_t offset)
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
    uint32_t data = io_in32(PCI_CONFIG_DATA);
    return data;
}

PRIVATE void pci_config_write(
    uint8_t bus,
    uint8_t dev,
    uint8_t func,
    uint8_t offset,
    uint32_t value)
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
    io_out32(PCI_CONFIG_DATA,value);
    return;
}

/// Header:
/// Offset | 31 - 24    |  23 - 16    | 15 - 8        | 7 - 0           |
/// 0x00   | Device ID                | Vendor ID                       |
/// 0x04   | Status                   | Command                         |
/// 0x08   | Class Code | Subclass    | Prog IF       | Revision ID     |
/// 0x0c   | BIST       | Header Type | Latency Timer | Cache Line Size |
/// 0x10   | ...                                                        |

PUBLIC uint32_t pci_dev_config_read(pci_device_t *dev,uint8_t offset)
{
    return pci_config_read(dev->bus,dev->device,dev->func,offset);
}

PUBLIC void pci_dev_config_write(pci_device_t *dev,uint8_t offset,uint32_t value)
{
    pci_config_write(dev->bus,dev->device,dev->func,offset,value);
    return;
}

PRIVATE uint8_t pci_read_header_type(uint8_t bus, uint8_t dev, uint8_t func)
{
    return (uint8_t)((pci_config_read(bus,dev,func,0x0c) >> 16) & 0xff);
}

PUBLIC uint8_t pci_dev_read_header_type(pci_device_t *dev)
{
    return pci_read_header_type(dev->bus,dev->device,dev->func);
}

PRIVATE uint16_t pci_read_vendor_id(uint8_t bus, uint8_t dev, uint8_t func)
{
    return (uint16_t)(pci_config_read(bus,dev,func,0x00) & 0xffff);
}

PUBLIC uint16_t pci_dev_read_vendor_id(pci_device_t *dev)
{
    return pci_read_vendor_id(dev->bus,dev->device,dev->func);
}

PRIVATE uint32_t pci_read_class_code(uint8_t bus, uint8_t dev, uint8_t func)
{
    return pci_config_read(bus,dev,func,0x08);
}

PUBLIC uint32_t pci_dev_read_class_code(pci_device_t *dev)
{
    return pci_read_class_code(dev->bus,dev->device,dev->func);
}

PRIVATE uint8_t pci_read_base_class_code(uint8_t bus, uint8_t dev, uint8_t func)
{
    return (uint8_t)((pci_read_class_code(bus,dev,func) >> 24) & 0xff);
}

PUBLIC uint8_t pci_dev_read_base_class_code(pci_device_t *dev)
{
    return pci_read_base_class_code(dev->bus,dev->device,dev->func);
}

PRIVATE uint8_t pci_read_sub_class_code(uint8_t bus, uint8_t dev, uint8_t func)
{
    return (uint8_t)((pci_read_class_code(bus,dev,func) >> 16) & 0xff);
}

PUBLIC uint8_t pci_dev_read_sub_class_code(pci_device_t *dev)
{
    return pci_read_sub_class_code(dev->bus,dev->device,dev->func);
}

PRIVATE uint8_t pci_read_secondary_bus_number(
    uint8_t bus,
    uint8_t dev,
    uint8_t func)
{
    return (uint8_t)((pci_config_read(bus,dev,func,0x18) >> 8) & 0xff);
}

PUBLIC uint8_t pci_dev_read_secondary_bus_number(pci_device_t *dev)
{
    return pci_read_secondary_bus_number(dev->bus,dev->device,dev->func);
}

PUBLIC uint64_t pci_dev_read_bar(pci_device_t *dev,uint8_t bar_index)
{
    if (bar_index > 5)
    {
        return 0;
    }
    uint8_t offset = bar_index * 4 + 0x10;
    uint64_t bar = pci_dev_config_read(dev,offset);
    uint32_t mask;
    mask = bar & 1 ? 0xfffffffc : 0xfffffff0;
    if ((bar & 0x4) == 0)
    {
        return bar & mask;
    }
    if (bar_index > 4)
    {
        return 0;
    }
    return ((uint64_t)pci_dev_config_read(dev,offset + 4) << 32) | (bar & mask);
}

PUBLIC uint8_t pci_dev_read_cap_point(pci_device_t *dev)
{
    return pci_dev_config_read(dev,0x34) & 0xff;
}

PRIVATE void add_device(pci_device_t *device)
{
    pci_devices[number_of_pci_device] = *device;
    number_of_pci_device++;
    // pr_log("\2 pci {%02x,%02x,%02x} class code: %06x: %s.\n",
    //         device->bus,
    //         device->device,
    //         device->func,
    //         ((device->class_code) >> 8) & 0xffffff,
    //         pci_dev_type_str(device));
}

PRIVATE void scan_func(uint8_t bus,uint8_t dev,uint8_t func);
PRIVATE void scan_device(uint8_t bus,uint8_t dev);
PRIVATE void scan_bus(uint8_t bus);

PRIVATE void scan_func(uint8_t bus,uint8_t dev,uint8_t func)
{
    uint8_t base_class;
    uint8_t sub_class;
    uint8_t secondary_bus;

    base_class = pci_read_base_class_code(bus,dev,func);
    sub_class  = pci_read_sub_class_code(bus,dev,func);

    uint8_t header_type = pci_read_header_type(bus,dev,func);
    uint32_t class_code = pci_read_class_code(bus,dev,func);
    pci_device_t device;
    device.bus         = bus;
    device.device      = dev;
    device.func        = func;
    device.header_type = header_type;
    device.class_code  = class_code;
    pci_dev_read_msi_info(&device);
    add_device(&device);

    if (base_class == 0x06 && sub_class == 0x04) // PCI to PCI bridge
    {
        secondary_bus = pci_read_secondary_bus_number(bus,dev,func);
        scan_bus(secondary_bus);
    }
}

PRIVATE void scan_device(uint8_t bus,uint8_t dev)
{
    uint8_t func = 0;
    uint16_t vendor_id = pci_read_vendor_id(bus,dev,func);
    if (vendor_id == 0xffff) // device dosen't exist
    {
        return;
    }
    uint8_t header_type = pci_read_header_type(bus,dev,func);
    if ((header_type & 0x80) == 0) // single-function device
    {
        scan_func(bus,dev,0);
        return;
    }
    // multi-functon device
    for (func = 0;func < 8;func++)
    {
        vendor_id = pci_read_vendor_id(bus,dev,func);
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
    uint8_t header_type = pci_read_header_type(0,0,0);
    if ((header_type & 0x80) == 0) // singal pci host controller
    {
        scan_bus(0);
        return;
    }
    for (func = 0;func < 8;func++)
    {
        if (pci_read_vendor_id(0,0,func) == 0xffff)
        {
            continue;
        }
        // bus = func;
        scan_bus(func);
    }
    return;
}

PUBLIC uint32_t pci_dev_count(
    uint8_t base_class,
    uint8_t sub_class,
    uint8_t prog_if)
{
    int res = 0;
    int i;
    for (i = 0;i < number_of_pci_device;i++)
    {
        uint32_t class_code = (pci_devices[i].class_code >> 8) & 0x00ffffff;
        if (((uint32_t)base_class << 16 | (uint32_t)sub_class << 8 | prog_if)
            == class_code)
        {
            res++;
        }
    }
    return res;
}

PUBLIC pci_device_t* pci_dev_match(
    uint8_t base_class,
    uint8_t sub_class,
    uint8_t prog_if,
    uint32_t index)
{
    uint32_t idx = 0;
    int i;
    for (i = 0;i < number_of_pci_device;i++)
    {
        uint32_t class_code = (pci_devices[i].class_code >> 8) & 0x00ffffff;
        if (((uint32_t)base_class << 16 | (uint32_t)sub_class << 8 | prog_if)
            == class_code)
        {
            if (idx == index)
            {
                return &pci_devices[i];
            }
            idx++;
        }
    }
    return NULL;
}

PUBLIC const char* pci_dev_type_str(pci_device_t *dev)
{
    uint32_t class_code = (pci_dev_read_class_code(dev) >> 8) & 0xffffff;
    switch (class_code)
    {
        case 0x010100: return "ISA Compatibility mode-only controller";
        case 0x010105: return "PCI native mode-only controller";
        case 0x01010a: return "ISA Compatibility mode controller";
        case 0x01010f: return "PCI native mode controller";
        case 0x010180: return "ISA Compatibility mode-only controller";
        case 0x010185: return "PCI native mode-only controller";
        case 0x01018a: return "ISA Compatibility mode controller";
        case 0x01018f: return "PCI native mode controller";

        case 0x010520: return "Single DMA";
        case 0x010530: return "Chained DMA";

        case 0x010600: return "Vendor Specific Interface";
        case 0x010601: return "AHCI 1.0";
        case 0x010602: return "Serial Storage Bus";

        case 0x010701: return "Serial Storage Bus";

        case 0x010801: return "NVMHCI";
        case 0x010802: return "NVM Express";

        case 0x030000: return "VGA Controller";
        case 0x030001: return "8514-Compatible Controller";

        case 0x080010: return "I/O APIC Interrupt Controller";
        case 0x080020: return "I/O(x) APIC Interrupt Controller";

        case 0x080200: return "Generic 8254-Compatible";
        case 0x080203: return "HPET";

        case 0x0c0320: return "EHCI Controller";
        case 0x0c0330: return "xHCI Controller";
        case 0x0c03fe: return "USB device";
    }
    return "Unknow";
}