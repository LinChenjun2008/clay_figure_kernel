#ifndef __ACPI_GUID_H__
#define __ACPI_GUID_H__


#define ACPI_TABLE_GUID \
  { \
    0xeb9d2d30, 0x2d88, 0x11d3, {0x9a, 0x16, 0x0, 0x90, 0x27, 0x3f, 0xc1, 0x4d } \
  }

#define EFI_ACPI_TABLE_GUID \
  { \
    0x8868e871, 0xe4f1, 0x11d3, {0xbc, 0x22, 0x0, 0x80, 0xc7, 0x3c, 0x88, 0x81 } \
  }

#define ACPI_10_TABLE_GUID      ACPI_TABLE_GUID
#define EFI_ACPI_20_TABLE_GUID  EFI_ACPI_TABLE_GUID

extern EFI_GUID  gEfiAcpiTableGuid;
extern EFI_GUID  gEfiAcpi10TableGuid;
extern EFI_GUID  gEfiAcpi20TableGuid;

#endif