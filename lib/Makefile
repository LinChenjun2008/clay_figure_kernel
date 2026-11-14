PROJECT_ROOT = ..
SRC_DIR      = ./src
OBJ_DIR      = $(PROJECT_ROOT)/build/objects/$(MODULE_NAME)
MODULE_NAME  = lib

SUB_DIR = .
SUB_DIR += user
SUB_DIR += user/arch/$(TARGET_ARCH)
SUB_DIR += user/mem

include ./tools_def.mk
include $(PROJECT_ROOT)/scripts/rules.mk

SKIP_COMMON_RULE := 1
include $(PROJECT_ROOT)/scripts/user/rules.mk

$(TARGET): $(OBJ)
	@$(ECHO) "AR      $@"
	@$(AR) $(ARFLAGS) $@ $(OBJ)
