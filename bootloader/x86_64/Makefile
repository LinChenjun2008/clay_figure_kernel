PROJECT_ROOT = ../..
SRC_DIR      = ./src

include ./tools_def.mk
include $(PROJECT_ROOT)/scripts/rules.mk

SRC = $(wildcard $(SRC_DIR)/*.c)

.PHONY: all
all: $(TARGET)

$(TARGET): $(SRC:.c=.o)
	@$(ECHO) "LD      $@"
	@"$(CC)" $(CFLAGS) -o $@ $^

.PHONY: clean
clean:
	-@$(RM) $(SRC:.c=.o) $(SRC:.c=.dep)
	-@$(RM) $(TARGET)

-include $(SRC:.c=.dep)