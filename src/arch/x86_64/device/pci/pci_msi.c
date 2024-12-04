/*
   Copyright 2024 LinChenjun

   本程序是自由软件
   修改和/或再分发依照 GNU GPL version 3 (or any later version)

*/

#include <kernel/global.h>
#include <device/pci.h> // pci_device_t,pci_dev_config_read,pci_dev_config_write
#include <device/pic.h> // apic
#include <log.h>

extern apic_t apic;

PUBLIC void pci_dev_read_msi_info(pci_device_t *dev)
{
    pci_msi_struct_t *msi = &dev->msi;
    uint8_t cap_addr = pci_dev_config_read(dev,0x34) & 0xff;
    while (cap_addr != 0)
    {
        uint32_t header = pci_dev_config_read(dev, cap_addr);
        if ((header & 0xff) == 5)
        {
            msi->msi_capable = TRUE;
            break;
        }
        cap_addr = (header & 0xff00) >> 8;
    }
    if (cap_addr == 0)
    {
        msi->msi_capable = FALSE;
        return;
    }
    msi->cap_addr     = cap_addr;
    msi->msg_ctrl     = (pci_dev_config_read(dev,msi->cap_addr) >> 16) & 0xffff;
    msi->msg_cnt      = 1 << ((msi->msg_ctrl & 0x0e) >> 1);
    msi->start_vector = 0;
    return;
}

PUBLIC void configure_msi(
    pci_device_t *dev,
    uint8_t trigger_mode,
    uint32_t delivery_mode,
    uint8_t vector,
    uint8_t num_vector_exponent)
{
    uint32_t msg_addr = apic.local_apic_address | apic.lapic_id[0] << 12;
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
        else if ((header & 0xff) == 0x11)
        {
            msix_cap_addr = cap_addr;
        }
        cap_addr = (header & 0xff00) >> 8;
    }

    if (msi_cap_addr)
    {
        uint32_t msi_cap = pci_dev_config_read(dev,msi_cap_addr);
        if (((msi_cap >> 17) & 0b0111) <= num_vector_exponent)
        {
            // multi msg enable = multi msg capable
            msi_cap &= 0x00700000;
            msi_cap |= ((msi_cap >> 17) & 0b0111) << 20;
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

        uint32_t msi_mask_bits    = pci_dev_config_read(dev,msi_cap_addr + 0x10);
        uint32_t msi_pending_bits = pci_dev_config_read(dev,msi_cap_addr + 0x14);
        if (msi_cap & (1 << 24)) // per-vector masking
        {
            // msi mask bits
            pci_dev_config_write(dev,msg_data_addr + 4,msi_mask_bits);
            // msi pending bits
            pci_dev_config_write(dev,msg_data_addr + 4,msi_pending_bits);
        }
        return;
    }
    else if (msix_cap_addr)
    {
        pr_log("\3 MSI-X not implemented.\n");
    }
}

PUBLIC status_t pci_dev_configure_msi(pci_device_t *dev,uint32_t irq,uint32_t count)
{
    if (!dev->msi.msi_capable)
    {
        pr_log("\3 PCI Device %x:%x:%x Not Support MSI.\n",
               dev->bus,
               dev->device,
               dev->func);
        return K_ERROR;
    }
    pci_msi_struct_t *msi = & dev->msi;
    if (count > 32 || count > msi->msg_cnt || ((count - 1) & count) != 0)
    {
        pr_log("\3 MSI: inavlid count.\n");
    }
    if (msi->configured_count != 0)
    {
        return K_ERROR;
    }
    // set vector
    msi->address_value = 0xfee00000 | (apic.lapic_id[0] << 12);
    msi->data_value    = irq;
    pci_dev_config_write(dev,
                         msi->cap_addr + 0x04,
                         msi->address_value & 0xffffffff);
    if (msi->msg_ctrl & 0x0080) // 64bit
    {
        pci_dev_config_write(dev,msi->cap_addr + 0x08,msi->address_value >> 32);
        pci_dev_config_write(dev,msi->cap_addr + 0x0c,msi->data_value);
    }
    else
    {
        pci_dev_config_write(dev,msi->cap_addr + 0x08,msi->data_value);
    }
    msi->msg_ctrl &= ~0x0070;
    msi->msg_ctrl |= (count - 1) << 4; // (count - 1) << 4

    uint32_t val = pci_dev_config_read(dev,msi->cap_addr);
    val |= msi->msg_ctrl << 16;
    pci_dev_config_write(dev,msi->cap_addr,val);
    msi->configured_count = count;
    return K_SUCCESS;
}

PUBLIC status_t pci_dev_enable_msi(pci_device_t *dev)
{
    pci_msi_struct_t *msi = &dev->msi;
    if (msi->configured_count == 0)
    {
        pr_log("\3 No MSI configured.\n");
        return K_ERROR;
    }
    uint32_t pci_command = pci_dev_config_read(dev,0x04);
    pci_command |= 1 << 10; // interrupt disable
    pci_dev_config_write(dev,0x04,pci_command);

    msi->msg_ctrl |= 0x01;
    pci_dev_config_write(dev,msi->cap_addr + 0x00,msi->msg_ctrl << 16);
    return K_SUCCESS;
}