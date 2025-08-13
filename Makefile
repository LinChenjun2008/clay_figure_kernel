PROJECT_DIR = .
SCRIPTS_DIR = $(PROJECT_DIR)/scripts
SRC_DIR = $(PROJECT_DIR)/src

include $(SCRIPTS_DIR)/tools_def.mk
include $(SCRIPTS_DIR)/target.mk

include $(SRC_DIR)/arch/src_list.mk
include $(SRC_DIR)/src_list.mk

ASM_SRC = $(filter %.S,$(SRC))
C_SRC   = $(filter %.c,$(SRC))

.PHONY: all
all:
	@$(ECHO) compiling...
	@$(MAKE) -C $(SRC_DIR)/arch/ bootloader
	@$(MAKE) -C $(SRC_DIR)/arch/ kernel
	@$(MAKE) -C $(SRC_DIR) kernel
	@$(MAKE) $(TARGET_KERNEL) update-initramfs
	@$(ECHO) done.

.PHONY: update-initramfs
update-initramfs:
	@$(MAKE) $(TARGET_INITFAMFS)

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
	-@$(RM) $(TMP_KERNEL_TARGET)
	-@$(RM) $(TARGET_KERNEL)

.PHONY: init
init:
	-$(MKDIR) "$(RUNNING_DIR)"
	-$(MKDIR) "$(ESP_DIR)"
	-$(MKDIR) "$(ESP_DIR)/EFI"
	-$(MKDIR) "$(ESP_DIR)/EFI/Boot"
	-$(MKDIR) "$(ESP_DIR)/Kernel"

$(TARGET_INITFAMFS): $(SRC_DIR)/config.txt
	@$(ECHO) make initramfs
	@"$(IMGCOPY)" $(IMGCOPY_FLAGS) > $(ESP_DIR)/Kernel/initramfs.img

$(TARGET_KERNEL): $(TMP_KERNEL_TARGET)
	@$(ECHO) making $@
	@"$(OBJCOPY)" $(OBJFLAGS) $^ $@

$(TMP_KERNEL_TARGET): $(ASM_SRC:S=o) $(C_SRC:c=o) $(KERNEL_LINKER_SCRIPT)
	@$(ECHO) linking [1/2]
	@"$(LD)" $(LDFLAGS) -o $@ $(ASM_SRC:S=o) $(C_SRC:c=o)
	@"$(NM)" -W -n $@ | "$(KALLSYMS)" > $@_sym.c
	@"$(CC)" $(KERNEL_FLAGS) -c -o $@_sym.o $@_sym.c
	@$(ECHO) linking [2/2]
	@"$(LD)" $(LDFLAGS) -o $@ $(ASM_SRC:S=o) $(C_SRC:c=o) $@_sym.o
	@"$(RM)" $@_sym.c $@_sym.o
