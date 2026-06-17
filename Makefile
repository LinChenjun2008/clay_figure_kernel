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
	@$(ECHO) ---[ Build ]---
	@$(ECHO) ESP: $(ESP_DIR)
	@$(MAKE) --no-print-directory -C $(SRC_DIR)/arch/ all
	@$(MAKE) --no-print-directory -C $(SRC_DIR) all
	@$(MAKE) --no-print-directory $(TARGET_INITRAMFS)
	@$(ECHO) ---[ Done  ]---

.PHONY: run
run: all
	-@"$(QEMU)" $(QEMU_FLAGS)

.PHONY: debug
debug: all
	-@"$(QEMU)" $(QEMU_FLAGS) -S -s

.PHONY: clean
clean:
	@$(MAKE) -C $(SRC_DIR)/arch/ clean
	@$(MAKE) -C $(SRC_DIR) clean
	-@"$(RM)" $(TARGET_INITRAMFS)
	-@"$(RM)" $(TARGET_KERNEL) $(TARGET_KERNEL:sys=sym) $(TARGET_KERNEL:sys=tmp)

.PHONY: init
init:
	-$(MKDIR) "$(BUILD_DIR)"
	-$(MKDIR) "$(ESP_DIR)"
	-$(MKDIR) "$(ESP_DIR)/efi"
	-$(MKDIR) "$(ESP_DIR)/efi/boot"
	-$(MKDIR) "$(ESP_DIR)/kernel"

$(TARGET_INITRAMFS): $(IMGCOPY_DEP)
	@"$(IMGCOPY)" $(IMGCOPY_FLAGS) > $@

$(TARGET_KERNEL): $(ASM_SRC:S=o) $(C_SRC:c=o) $(KERNEL_LINKER_SCRIPT)
	@$(ECHO) "LD      $(@:sys=tmp)"
	@"$(LD)" $(LDFLAGS) -o $(@:sys=tmp) $(ASM_SRC:S=o) $(C_SRC:c=o)
	@"$(NM)" -W -n $(@:sys=tmp) | "$(KALLSYMS)" > $(@:sys=c)
	@"$(CC)" $(KERNEL_FLAGS) -c -o $(@:sys=o) $(@:sys=c)
	@$(ECHO) "LD      $(@:sys=tmp)"
	@"$(LD)" $(LDFLAGS) -o $(@:sys=tmp) $(ASM_SRC:S=o) $(C_SRC:c=o) $(@:sys=o)
	@$(ECHO) "OBJCOPY $(@:sys=sym)"
	@"$(OBJCOPY)" --only-keep-debug $(@:sys=tmp) $(@:sys=sym)
	@$(ECHO) "OBJCOPY $@"
	@"$(OBJCOPY)" -S -R ".eh_frame" -R ".comment" $(@:sys=tmp) $@
	@"$(RM)" $(@:sys=o) $(@:sys=c)