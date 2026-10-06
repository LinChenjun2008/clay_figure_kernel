PROJECT_ROOT = ../..
SRC_DIR      = ./src
OBJ_DIR      = $(PROJECT_ROOT)/build/objects/$(MODULE_NAME)
MODULE_NAME  = bootloader

include ./tools_def.mk
include $(PROJECT_ROOT)/scripts/rules.mk

SRC = $(wildcard $(SRC_DIR)/*.c)

OBJ      = $(addprefix $(OBJ_DIR)/,$(SRC:.c=.o))
DEP      = $(OBJ:.o=.dep)

.PHONY: all
all: $(TARGET)

$(TARGET): $(OBJ)
	@$(ECHO) "LD      $@"
	@$(CC) $(CFLAGS) -o $@ $^

.PHONY: clean
clean:
	@$(RM) $(OBJ) $(DEP)
	@$(RM) $(TARGET)

-include $(DEP)