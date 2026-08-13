PROJECT_ROOT = .
SCRIPTS_DIR = $(PROJECT_ROOT)/scripts
BUILD_DIR   = $(PROJECT_ROOT)/build
ESP_DIR     = $(BUILD_DIR)/esp

include $(SCRIPTS_DIR)/tools_def.mk

.PHONY: all
all:
	@$(ECHO) ---[ Build ]---
	@$(MAKE) -C bootloader/$(TARGET_ARCH) all
	@$(MAKE) -C kernel TARGET_ARCH=$(TARGET_ARCH) all
	@$(ECHO) ---[ Done  ]---

.PHONY: clean
clean:
	@$(ECHO) ---[ Clean ]---
	@$(MAKE) -C bootloader/$(TARGET_ARCH) clean
	@$(MAKE) -C kernel TARGET_ARCH=$(TARGET_ARCH) clean
	@$(ECHO) ---[ Done  ]---

.PHONY: init
init:
	@$(ECHO) ---[ Init  ]---
	-@$(MKDIR) "$(BUILD_DIR)"
	-@$(MKDIR) "$(ESP_DIR)"
	-@$(MKDIR) "$(ESP_DIR)/efi"
	-@$(MKDIR) "$(ESP_DIR)/efi/boot"
	-@$(MKDIR) "$(ESP_DIR)/kernel"
	@$(ECHO) ---[ Done  ]---

.PHONY: run
run: all
	-@$(QEMU) $(QEMU_FLAGS)