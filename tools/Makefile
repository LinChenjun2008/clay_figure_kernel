PROJECT_ROOT = ..
SRC_DIR      = ./
OBJ_DIR      = $(PROJECT_ROOT)/build/objects/$(MODULE_NAME)
MODULE_NAME  = imgcopy

include $(PROJECT_ROOT)/scripts/tools_def.mk

CFLAGS = -I$(PROJECT_ROOT)/include/

include $(PROJECT_ROOT)/scripts/rules.mk

C_SRC = $(wildcard ./*.c)

A_OBJ = $(call to_obj,$(A_SRC),S)
C_OBJ = $(call to_obj,$(C_SRC),c)
A_DEP = $(patsubst %.o,%.dep,$(A_OBJ))
C_DEP = $(patsubst %.o,%.dep,$(C_OBJ))

OBJ   = $(A_OBJ) $(C_OBJ)
DEP   = $(A_DEP) $(C_DEP)

TARGET = $(PROJECT_ROOT)/build/tools/imgcopy/imgcopy

$(TARGET): $(OBJ)
	@$(MKDIR) -p $(@D)
	@$(ECHO) "LD      $@"
	@$(CC) -o $@ $(OBJ)

.PHONY: all
all: $(TARGET)

.PHONY: clean
clean:
	@$(RM) $(OBJ) $(DEP)
	@$(RM) $(TARGET)

-include $(DEP)