RUNNING_DIR      = $(SRC_DIR)/../../run
ESP_DIR          = $(RUNNING_DIR)/esp
TARGET_BOOTLOADER = $(ESP_DIR)/EFI/Boot/BootX64.efi
TARGET_KERNEL     = $(ESP_DIR)/Kernel/clfgkrnl.sys
TARGET_INITFAMFS  = $(ESP_DIR)/Kernel/initramfs.img

TARGET_ARCH       = x86_64
SMP_CORES         = 4
MEMORY            = 256M

TMP_KERNEL_TARGET = .tmp_kernel.o
