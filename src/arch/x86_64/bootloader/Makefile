PROJECT_DIR = ../../../../
SCRIPTS_DIR = $(PROJECT_DIR)/scripts
SRC_DIR = $(PROJECT_DIR)/src

TOOLS_DEF = bootloader
include $(SCRIPTS_DIR)/tools_def.mk
include $(SCRIPTS_DIR)/target.mk
include src_list.mk

$(TARGET_BOOTLOADER): $(SRC)
	@$(ECHO) making $@
	@"$(MINGW64_GCC)" $(CFLAGS) -o $@ $^