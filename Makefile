PROJECT_DIR = $(abspath .)
SCRIPTS_DIR = $(PROJECT_DIR)/scripts
SRC_DIR = $(abspath $(PROJECT_DIR)/src)

include $(SCRIPTS_DIR)/tools_def.mk
include $(SCRIPTS_DIR)/target.mk

include $(SRC_DIR)/arch/src_list.mk
ASM_SRC := $(filter %.S,$(SRC))
C_SRC   := $(filter %.c,$(SRC))

include $(SRC_DIR)/src_list.mk
ASM_SRC += $(filter %.S,$(SRC))
C_SRC   += $(filter %.c,$(SRC))

.PHONY: all
all:
	@$(ECHO) compiling...
	@$(MAKE) -C $(SRC_DIR)/arch/ all
	@$(MAKE) -C $(SRC_DIR) all
	@$(MAKE) $(TARGET_KERNEL)
	@$(MAKE) $(TARGET_INITRAMFS)
	@$(ECHO) done.

.PHONY: run
run: all
	-@"$(QEMU)" $(QEMU_FLAGS)

.PHONY: debug
debug: all
	-@"$(QEMU)" -S -s $(QEMU_FLAGS)

.PHONY: clean
clean:
	@$(MAKE) -C $(SRC_DIR)/arch clean
	@$(MAKE) -C $(SRC_DIR) clean
	-@$(RM) $(TARGET_KERNEL)

.PHONY: init
init:
	-$(MKDIR) "$(RUNNING_DIR)"
	-$(MKDIR) "$(ESP_DIR)"
	-$(MKDIR) "$(ESP_DIR)/EFI"
	-$(MKDIR) "$(ESP_DIR)/EFI/Boot"
	-$(MKDIR) "$(ESP_DIR)/Kernel"

$(TARGET_INITRAMFS): $(SRC_DIR)/config.txt $(TARGET_KERNEL)
	@$(ECHO) updating initramfs...
	@"$(IMGCOPY)" $(IMGCOPY_FLAGS) > $(ESP_DIR)/Kernel/initramfs.img

$(TARGET_KERNEL): $(ASM_SRC:S=o) $(C_SRC:c=o) $(KERNEL_LINKER_SCRIPT)
	@$(ECHO) linking [1/2]
	@"$(LD)" $(LDFLAGS) -o $@ $(ASM_SRC:S=o) $(C_SRC:c=o)
	@"$(NM)" -W -n $@ | "$(KALLSYMS)" > $@_sym.c
	@"$(CC)" $(KERNEL_FLAGS) -c -o $@_sym.o $@_sym.c
	@$(ECHO) linking [2/2]
	@"$(LD)" $(LDFLAGS) -o $@ $(ASM_SRC:S=o) $(C_SRC:c=o) $@_sym.o
	@"$(RM)" $@_sym.c $@_sym.o
