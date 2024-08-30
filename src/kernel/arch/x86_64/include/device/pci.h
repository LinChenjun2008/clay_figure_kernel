/*
   Copyright 2024 LinChenjun

This file is part of Clay Figure Kernel.

Clay Figure Kernel is free software: you can redistribute it and/or modify
it underthe terms of the GNU General Public License as published by
the Free Software Foundation,either version 3 of the License, or (at your option)
any later version.

Clay Figure Kernel is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY;without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Clay Figure Kernel.If not, see
<https://www.gnu.org/licenses/>.

本文件是Clay Figure Kernel的一部分。

Clay Figure Kernel 是自由软件：你可以再分发之和/或依照由自由软件基金会发布的
GNU 通用公共许可证修改之，无论是版本 3 许可证，还是（按你的决定）任何以后版都可以。

发布 Clay Figure Kernel 是希望它能有用，但是并无保障;
甚至连可销售和符合某个特定的目的都不保证。请参看GNU 通用公共许可证，了解详情。

你应该随程序获得一份 GNU 通用公共许可证的复本。如果没有，请看
<https://www.gnu.org/licenses/>。  */

#ifndef __PCI_H__
#define __PCI_H__

typedef struct pci_msi_struct_s
{
    bool     msi_capable;
    uint8_t  cap_addr; // capability_offset
    uint32_t msg_cnt;  // message count
    uint32_t configured_count;
    uint32_t start_vector;
    uint16_t msg_ctrl; // control value
    uint32_t data_value;
    uint64_t address_value;
} pci_msi_struct_t;

typedef struct pci_device_s
{
    uint8_t  bus;
    uint8_t  device;
    uint8_t  func;
    uint8_t  header_type;
    uint32_t class_code;
    pci_msi_struct_t msi;
} pci_device_t;

PUBLIC uint32_t pci_dev_config_read(pci_device_t *dev,uint8_t offset);
PUBLIC void pci_dev_config_write(
    pci_device_t *dev,
    uint8_t offset,
    uint32_t value);

PUBLIC uint8_t pci_dev_read_header_type(pci_device_t *dev);
PUBLIC uint16_t pci_dev_read_vendor_id(pci_device_t *dev);
PUBLIC uint32_t pci_dev_read_class_code(pci_device_t *dev);
PUBLIC uint8_t pci_dev_read_base_class_code(pci_device_t *dev);
PUBLIC uint8_t pci_dev_read_sub_class_code(pci_device_t *dev);
PUBLIC uint8_t pci_dev_read_secondary_bus_number(pci_device_t *dev);

PUBLIC uint64_t pci_dev_read_bar(pci_device_t *dev,uint8_t bar_index);
PUBLIC uint8_t pci_dev_read_cap_point(pci_device_t *dev);

PUBLIC void pci_scan_all_bus();

PUBLIC uint32_t pci_dev_count(
    uint8_t base_class,
    uint8_t sub_class,
    uint8_t prog_if);
PUBLIC pci_device_t* pci_dev_match(
    uint8_t base_class,
    uint8_t sub_class,
    uint8_t prog_if,
    uint32_t index);

PUBLIC const char* pci_dev_type_str(pci_device_t *dev);

// pci_msi.c
PUBLIC void pci_dev_read_msi_info(pci_device_t *dev);

PUBLIC void configure_msi
(
    pci_device_t *dev,
    uint8_t trigger_mode,
    uint32_t delivery_mode,
    uint8_t vector,
    uint8_t num_vector_exponent
);

PUBLIC status_t pci_dev_configure_msi(
    pci_device_t *dev,
    uint32_t irq,
    uint32_t count);
PUBLIC status_t pci_dev_enable_msi(pci_device_t *dev);

#endif