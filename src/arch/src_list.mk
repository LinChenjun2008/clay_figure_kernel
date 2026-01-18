
SUB_DIR = .
SUB_DIR += intr

SUB_DIR += device
SUB_DIR += device/cpu
SUB_DIR += device/cpu/smp
SUB_DIR += device/keyboard
SUB_DIR += device/keyboard/service
SUB_DIR += device/pic
SUB_DIR += device/pci
SUB_DIR += device/timer
SUB_DIR += device/usb/hid
SUB_DIR += device/usb/service
SUB_DIR += device/usb/xhci
SUB_DIR += lib
SUB_DIR += mem
SUB_DIR += sync
SUB_DIR += syscall
SUB_DIR += task

SEARCH_DIR = $(SRC_DIR)/arch/$(TARGET_ARCH)

SRC := $(foreach DIR,$(SUB_DIR),$(abspath $(wildcard $(SEARCH_DIR)/$(DIR)/*.S)))
SRC += $(foreach DIR,$(SUB_DIR),$(abspath $(wildcard $(SEARCH_DIR)/$(DIR)/*.c)))
