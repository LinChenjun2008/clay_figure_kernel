PROJECT_DIR = ..
SCRIPTS_DIR = $(PROJECT_DIR)/scripts
SRC_DIR = $(PROJECT_DIR)/src

include $(SCRIPTS_DIR)/tools_def.mk
include $(SCRIPTS_DIR)/target.mk
include src_list.mk

ASM_SRC = $(filter %.S,$(SRC))
C_SRC   = $(filter %.c,$(SRC))

.PHONY: all
all: kernel

include $(SCRIPTS_DIR)/rules.mk

.PHONY: kernel
kernel: $(ASM_SRC:S=o) $(C_SRC:c=o)

.PHONY: clean
clean:
	@$(RM) $(ASM_SRC:S=o) $(C_SRC:c=o)
	@$(RM) $(ASM_SRC:S=dep) $(C_SRC:c=dep)

-include $(ASM_SRC:S=dep)
-include $(C_SRC:c=dep)
