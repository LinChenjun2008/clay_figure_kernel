RUNNING_DIR       = $(abspath $(PROJECT_DIR)/../run)
ESP_DIR           = $(abspath $(RUNNING_DIR)/esp)
TARGET_BOOTLOADER = $(abspath $(ESP_DIR)/EFI/Boot/BootX64.efi)
TARGET_KERNEL     = $(abspath $(ESP_DIR)/Kernel/clfgkrnl.sys)
TARGET_INITRAMFS  = $(abspath $(ESP_DIR)/Kernel/initramfs.img)

TARGET_ARCH       = x86_64
SMP_CORES         = 4
MEMORY            = 256M
