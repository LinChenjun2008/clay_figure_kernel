BUILD_DIR         = $(abspath $(PROJECT_DIR)/build)
ESP_DIR           = $(abspath $(BUILD_DIR)/esp)
TARGET_KERNEL     = $(abspath $(BUILD_DIR)/clfgkrnl.sys)
TARGET_BOOTLOADER = $(abspath $(ESP_DIR)/efi/boot/bootx64.efi)
TARGET_INITRAMFS  = $(abspath $(ESP_DIR)/kernel/initramfs.img)

TARGET_ARCH       = x86_64
SMP_CORES         = 4
MEMORY            = 256M
