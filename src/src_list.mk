SUB_DIR = .
SUB_DIR += lib
SUB_DIR += mem
SUB_DIR += print
SUB_DIR += ramfs
SUB_DIR += std
SUB_DIR += task

SEARCH_DIR = $(SRC_DIR)

SRC := $(foreach DIR,$(SUB_DIR),$(abspath $(wildcard $(SEARCH_DIR)/$(DIR)/*.S)))
SRC += $(foreach DIR,$(SUB_DIR),$(abspath $(wildcard $(SEARCH_DIR)/$(DIR)/*.c)))