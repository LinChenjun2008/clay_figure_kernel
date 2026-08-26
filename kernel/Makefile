PROJECT_ROOT = ..
SRC_DIR      = ./src

include ./tools_def.mk
include $(PROJECT_ROOT)/scripts/rules.mk

include src_list.mk

.PHONY: all
all: $(TARGET)

$(TARGET): $(C_SRC:.c=.o) $(A_SRC:.S=.o) $(LD_SCRIPT)
	@$(ECHO) "LD      $@"
	@"$(LD)" $(LDFLAGS) -o $@ $(C_SRC:.c=.o) $(A_SRC:.S=.o)

.PHONY: clean
clean:
	-@$(RM) $(C_SRC:.c=.o) $(C_SRC:.c=.dep)
	-@$(RM) $(A_SRC:.S=.o) $(A_SRC:.S=.dep)
	-@$(RM) $(TARGET)

-include $(C_SRC:.c=.dep)
-include $(A_SRC:.S=.dep)
