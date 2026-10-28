SUB_DIR = .
SUB_DIR += common
SUB_DIR += user
SUB_DIR += user/arch/$(TARGET_ARCH)
SUB_DIR += user/mem

C_SRC = $(foreach DIR,$(SUB_DIR),$(wildcard $(SRC_DIR)/$(DIR)/*.c))
A_SRC = $(foreach DIR,$(SUB_DIR),$(wildcard $(SRC_DIR)/$(DIR)/*.S))