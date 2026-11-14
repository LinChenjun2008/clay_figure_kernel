MEMORY      = 512M
SMP_CORES   = 2
TARGET_ARCH = x86_64

include $(PROJECT_ROOT)/scripts/tools_def.mk

IMGCOPY   = $(PROJECT_ROOT)/build/tools/imgcopy/imgcopy
INTIRAMFS = $(ESP_DIR)/initramfs.img

QEMU_FLAGS = -m $(MEMORY) -bios $(OVMF) \
 -smp $(SMP_CORES),cores=$(SMP_CORES),threads=1,sockets=1 \
 -drive file=fat:rw:$(ESP_DIR),index=0,format=vvfat -net none \
 -usb \
 -device nec-usb-xhci,id=xhci \
 -device usb-mouse \
 -no-shutdown -no-reboot \
 -chardev stdio,mux=on,id=com1 \
 -serial chardev:com1