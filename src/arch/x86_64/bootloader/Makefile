PROJECT_DIR = $(abspath ../../../..)
SCRIPTS_DIR = $(PROJECT_DIR)/scripts
SRC_DIR = $(abspath $(PROJECT_DIR)/src)

include $(SCRIPTS_DIR)/tools_def.mk
include $(SCRIPTS_DIR)/target.mk

SRC = $(abspath $(wildcard ./*.c))

$(TARGET_BOOTLOADER): $(SRC:c=o)
	@$(ECHO) "LD      $@"
	@"$(CC)" $(CFLAGS) -o $@ $^

include $(SCRIPTS_DIR)/rules.mk

.PHONY: clean
clean:
	-@"$(RM)" $(SRC:c=o) $(SRC:c=dep)
	-@"$(RM)" $(TARGET_BOOTLOADER)

-include $(SRC:c=dep)