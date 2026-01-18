PROJECT_DIR = $(abspath ../../../..)
SCRIPTS_DIR = $(PROJECT_DIR)/scripts
SRC_DIR = $(abspath $(PROJECT_DIR)/src)

include $(SCRIPTS_DIR)/tools_def.mk
include $(SCRIPTS_DIR)/target.mk
include src_list.mk

$(TARGET_BOOTLOADER): $(SRC)
	@$(ECHO) making $@
	@"$(CC)" $(CFLAGS) -o $@ $^