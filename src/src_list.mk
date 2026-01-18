SUB_DIR = .
SUB_DIR += elf
SUB_DIR += graphic
SUB_DIR += kernel
SUB_DIR += kernel/service
SUB_DIR += lib
SUB_DIR += mem
SUB_DIR += mem/service
SUB_DIR += ramfs
SUB_DIR += softirq
SUB_DIR += service
SUB_DIR += service/tick
SUB_DIR += service/view
SUB_DIR += ulib

SEARCH_DIR = $(SRC_DIR)

SRC := $(foreach DIR,$(SUB_DIR),$(abspath $(wildcard $(SEARCH_DIR)/$(DIR)/*.S)))
SRC += $(foreach DIR,$(SUB_DIR),$(abspath $(wildcard $(SEARCH_DIR)/$(DIR)/*.c)))
