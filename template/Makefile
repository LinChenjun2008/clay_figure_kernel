PROJECT_ROOT = ..
SRC_DIR      = ./src
OBJ_DIR      = $(PROJECT_ROOT)/build/objects/$(MODULE_NAME)
MODULE_NAME  = module_name

include ./tools_def.mk
include $(PROJECT_ROOT)/scripts/rules.mk

include src_list.mk

A_OBJ = $(call to_obj,$(A_SRC),S)
C_OBJ = $(call to_obj,$(C_SRC),c)
A_DEP = $(patsubst %.o,%.dep,$(A_OBJ))
C_DEP = $(patsubst %.o,%.dep,$(C_OBJ))

OBJ   = $(A_OBJ) $(C_OBJ)
DEP   = $(A_DEP) $(C_DEP)

.PHONY: all
all: $(TARGET)

$(TARGET): $(OBJ)
	@$(ECHO) "LD      $@"
	@$(LD) $(LDFLAGS) -o $@ $^ -L$(PROJECT_ROOT)/build/lib/ -lsys

.PHONY: clean
clean:
	@$(RM) $(OBJ) $(DEP)
	@$(RM) $(TARGET)

-include $(DEP)
