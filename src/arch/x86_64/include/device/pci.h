// SPDX-License-Identifier: GPL-3.0-or-later
/**
 * Copyright (C) 2024-2025 LinChenjun
 */

#ifndef __PCI_H__
#define __PCI_H__

#define PCI_CONFIG_ADDRESS 0x0cf8
#define PCI_CONFIG_DATA    0x0cfc

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
    uint8_t          bus;
    uint8_t          device;
    uint8_t          func;
    uint8_t          header_type;
    uint32_t         class_code;
    pci_msi_struct_t msi;
} pci_device_t;

PUBLIC uint32_t pci_dev_config_read(pci_device_t *dev, uint8_t offset);
PUBLIC void
pci_dev_config_write(pci_device_t *dev, uint8_t offset, uint32_t value);

PUBLIC uint8_t  pci_dev_read_header_type(pci_device_t *dev);
PUBLIC uint16_t pci_dev_read_vendor_id(pci_device_t *dev);
PUBLIC uint32_t pci_dev_read_class_code(pci_device_t *dev);
PUBLIC uint8_t  pci_dev_read_base_class_code(pci_device_t *dev);
PUBLIC uint8_t  pci_dev_read_sub_class_code(pci_device_t *dev);
PUBLIC uint8_t  pci_dev_read_secondary_bus_number(pci_device_t *dev);

PUBLIC uint64_t pci_dev_read_bar(pci_device_t *dev, uint8_t bar_index);
PUBLIC uint8_t  pci_dev_read_cap_point(pci_device_t *dev);

PUBLIC void pci_scan_all_bus(void);

/**
 * @brief 获取满足条件的PCI设备个数
 * @param base_class Base class code
 * @param sub_class Sub class code
 * @param prog_if Programming Interface byte
 */
PUBLIC uint32_t
pci_dev_count(uint8_t base_class, uint8_t sub_class, uint8_t prog_if);

/**
 * @brief 匹配满足条件的PCI设备
 * @param base_class Base class code
 * @param sub_class Sub class code
 * @param prog_if Programming Interface byte
 * @param index PCI设备索引(满足条件的第几个PCI设备)
 */
PUBLIC pci_device_t *pci_dev_match(
    uint8_t  base_class,
    uint8_t  sub_class,
    uint8_t  prog_if,
    uint32_t index
);

PUBLIC const char *pci_dev_type_str(pci_device_t *dev);



// pci_msi.c

/**
 * @brief 读取PCI设备的MSI信息
 * @param dev 读取这个设备的MSI信息
 */
PUBLIC void pci_dev_read_msi_info(pci_device_t *dev);

/**
 * @brief 配置PCI设备的MSI信息
 */
PUBLIC void configure_msi(
    pci_device_t *dev,
    uint8_t       trigger_mode,
    uint32_t      delivery_mode,
    uint8_t       vector,
    uint8_t       num_vector_exponent
);

/**
 * @brief 配置PCI设备的MSI中断
 */
PUBLIC status_t
pci_dev_configure_msi(pci_device_t *dev, uint32_t irq, uint32_t count);

/**
 * @brief 开启MSI
 */
PUBLIC status_t pci_dev_enable_msi(pci_device_t *dev);

#endif