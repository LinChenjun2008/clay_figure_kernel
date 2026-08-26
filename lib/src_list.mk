SUB_DIR = .
SUB_DIR += arch/$(TARGET_ARCH)

C_SRC = $(foreach DIR,$(SUB_DIR),$(wildcard $(SRC_DIR)/$(DIR)/*.c))
A_SRC = $(foreach DIR,$(SUB_DIR),$(wildcard $(SRC_DIR)/$(DIR)/*.S))