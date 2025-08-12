RUNNING_PATH      = $(SRC_PATH)/../../run
ESP_PATH          = $(RUNNING_PATH)/esp
TARGET_BOOTLOADER = $(ESP_PATH)/EFI/Boot/BootX64.efi
TARGET_KERNEL     = $(ESP_PATH)/Kernel/clfgkrnl.sys
TARGET_INITFAMFS  = $(ESP_PATH)/Kernel/initramfs.img

TARGET_ARCH       = x86_64
SMP_CORES         = 4
MEMORY            = 256M

TMP_KERNEL_TARGET = .tmp_kernel
