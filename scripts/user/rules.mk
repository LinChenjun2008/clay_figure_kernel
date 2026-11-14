include $(PROJECT_ROOT)/scripts/user/src_list.mk

A_OBJ = $(call to_obj,$(A_SRC),S)
C_OBJ = $(call to_obj,$(C_SRC),c)
A_DEP = $(patsubst %.o,%.dep,$(A_OBJ))
C_DEP = $(patsubst %.o,%.dep,$(C_OBJ))

OBJ = $(A_OBJ) $(C_OBJ)
DEP = $(A_DEP) $(C_DEP)

.PHONY: all
all: $(TARGET)

.PHONY: clean
clean:
	@$(RM) $(OBJ) $(DEP)
	@$(RM) $(TARGET)

ifndef SKIP_COMMON_RULE
$(TARGET): $(OBJ)
	@$(ECHO) "LD      $@"
	@$(LD) $(LDFLAGS) -o $@ $^ -L$(PROJECT_ROOT)/build/lib/ -lsys
endif

-include $(DEP)
