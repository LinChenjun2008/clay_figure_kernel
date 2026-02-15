SUB_DIR = .
SUB_DIR += desc
SUB_DIR += driver/acpi
SUB_DIR += driver/pic
SUB_DIR += driver/timer
SUB_DIR += init
SUB_DIR += interrupt
SUB_DIR += mem
SUB_DIR += mp
SUB_DIR += sync
SUB_DIR += task

SEARCH_DIR = $(SRC_DIR)/arch/$(TARGET_ARCH)

SRC := $(foreach DIR,$(SUB_DIR),$(abspath $(wildcard $(SEARCH_DIR)/$(DIR)/*.S)))
SRC += $(foreach DIR,$(SUB_DIR),$(abspath $(wildcard $(SEARCH_DIR)/$(DIR)/*.c)))