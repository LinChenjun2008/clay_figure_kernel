PROJECT_ROOT = ..
SRC_DIR      = ./src

include ./tools_def.mk
include $(PROJECT_ROOT)/scripts/tools_def.mk
include $(PROJECT_ROOT)/scripts/rules.mk

include src_list.mk

.PHONY: all
all: $(TARGET)

$(TARGET): $(C_SRC:.c=.o)
	@$(ECHO) "LD      $@"
	@"$(LD)" $(LDFLAGS) -o $@ $^ -L$(PROJECT_ROOT)/build/lib/ -lsys

.PHONY: clean
clean:
	-@$(RM) $(C_SRC:.c=.o) $(C_SRC:.c=.dep)
	-@$(RM) $(TARGET)

-include $(C_SRC:.c=.dep)