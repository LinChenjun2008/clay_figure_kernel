#include <kernel/global.h>
#include <device/pci.h>
#include <io.h>
#include <device/pic.h>

#include <log.h>

extern apic_t apic_struct;

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
    uint32_t data = io_in32(PCI_CONFIG_DATA);
    return data;
}

PRIVATE void pci_config_write(uint8_t bus, uint8_t dev, uint8_t func, uint8_t offset,uint32_t value)
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

PRIVATE uint16_t pci_read_vendor_id(uint8_t bus, uint8_t dev, uint8_t func)
{
    return (uint16_t)(pci_config_read(bus,dev,func,0x00) & 0xffff);
}

PRIVATE uint32_t pci_read_class_code(uint8_t bus, uint8_t dev, uint8_t func)
{
    return pci_config_read(bus,dev,func,0x08);
}

PRIVATE uint8_t pci_read_base_class_code(uint8_t bus, uint8_t dev, uint8_t func)
{
    return (uint8_t)((pci_read_class_code(bus,dev,func) >> 24) & 0xff);
}

PRIVATE uint8_t pci_read_sub_class_code(uint8_t bus, uint8_t dev, uint8_t func)
{
    return (uint8_t)((pci_read_class_code(bus,dev,func) >> 16) & 0xff);
}

PRIVATE uint8_t pci_read_secondary_bus_number(uint8_t bus, uint8_t dev, uint8_t func)
{
    return (uint8_t)((pci_config_read(bus,dev,func,0x18) >> 8) & 0xff);
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
    pci_device_t device = {bus,dev,func,header_type,class_code};
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
    if ((header_type & 0x80) == 0)// singal pci host controller
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

PUBLIC pci_device_t* pci_dev_match(uint8_t base_class,uint8_t sub_class,uint8_t prog_if)
{
    int i;
    for (i = 0;i < number_of_pci_device;i++)
    {
        uint32_t class_code = (pci_devices[i].class_code >> 8) & 0x00ffffff;
        if (((uint32_t)base_class << 16 | (uint32_t)sub_class << 8 | prog_if) == class_code)
        {
            return &pci_devices[i];
        }
    }
    return NULL;
}

PUBLIC void configure_msi
(
    pci_device_t *dev,
    uint8_t trigger_mode,
    uint32_t delivery_mode,
    uint8_t vector,
    uint8_t num_vector_exponent
)
{
    uint32_t msg_addr = apic_struct.local_apic_address | apic_struct.lapic_id[0] << 12;
    uint32_t msg_data = delivery_mode << 8 | vector;
    if (trigger_mode == 1)
    {
        msg_data |= 0xc00;
    }
    uint8_t cap_addr = pci_dev_config_read(dev,0x34) & 0xffu;
    uint8_t msi_cap_addr = 0;
    uint8_t msix_cap_addr = 0;
    while (cap_addr != 0)
    {
        uint32_t header = pci_dev_config_read(dev, cap_addr);
        if ((header & 0xff) == 5)
        {
            msi_cap_addr = cap_addr;
        }
        else if ((header & 0xff) == 17)
        {
            msix_cap_addr = cap_addr;
        }
        cap_addr = (header & 0xff00) >> 8;
    }

    if (msi_cap_addr)
    {
        uint32_t msi_cap = pci_dev_config_read(dev,msi_cap_addr);
        if (((msi_cap >> 16) & 0b0111) /* multi msg capable */ <= num_vector_exponent)
        {
            // multi msg enable = multi msg capable
            msi_cap &= 0x00700000;
            msi_cap |= (msi_cap & 0x000e0000) << 3;
        }
        else
        {
            msi_cap &= 0x00700000;
            msi_cap |= num_vector_exponent << 20;
        }
        // msi enable
        msi_cap |= (1 << 16);

        // write
        pci_dev_config_write(dev,msi_cap_addr,msi_cap);
        pci_dev_config_write(dev,msi_cap_addr + 4,msg_addr);

        uint8_t msg_data_addr = msi_cap_addr + 8;
        if (msi_cap & (1 << 23)) // 64 bit
        {
            pci_dev_config_write(dev,msi_cap_addr + 8,0);
            msg_data_addr = msi_cap_addr + 12;
        }
        pci_dev_config_write(dev,msg_data_addr,msg_data);

        if (msi_cap & (1 << 24)) // per-vector masking
        {
            // msi mask bits
            pci_dev_config_write(dev,msg_data_addr + 4,pci_dev_config_read(dev,msi_cap_addr + 0x10));
            // msi pending bits
            pci_dev_config_write(dev,msg_data_addr + 4,pci_dev_config_read(dev,msi_cap_addr + 0x14));
        }
        pr_log("\1Congiure MSI done.\n");
        return;
    }
    else if (msix_cap_addr)
    {
        pr_log("\3MSI-X not implemented.\n");
    }
}